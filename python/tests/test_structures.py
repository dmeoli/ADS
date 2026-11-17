"""The succinct structures, the bitmaps, the filters and the heaps of ADS
from Python, against NumPy, pyroaring and heapq."""

import heapq
import pickle

import numpy as np
import pytest

import ads

rng = np.random.default_rng(11)


def test_bit_vector_and_rank_select(tmp_path):
    bits = rng.random(10000) < 0.3
    b = ads.BitVector.from_bools(bits)
    assert len(b) == len(bits) and b.count() == bits.sum()
    assert np.array_equal(b.to_numpy(), bits)
    # the words are those of numpy.packbits, little-endian
    packed = np.packbits(bits, bitorder="little")
    packed = np.pad(packed, (0, (-len(packed)) % 8)).view(np.uint64)
    assert np.array_equal(b.words, packed)
    assert ads.BitVector.from_words(packed, len(bits)) == b
    rs = ads.RankSelect(b)
    ranks = np.concatenate([[0], np.cumsum(bits)])
    pos = np.arange(len(bits) + 1, dtype=np.uint64)
    assert np.array_equal(rs.rank1(pos), ranks)
    ones = np.flatnonzero(bits)
    assert np.array_equal(rs.select1(np.arange(1, len(ones) + 1,
                                               dtype=np.uint64)), ones)
    assert rs.select0(1) == np.flatnonzero(~bits)[0]
    with pytest.raises(IndexError):
        rs.select1(0)
    p = tmp_path / "rs.ads"
    rs.store(p)
    for r in (ads.RankSelect.restore(p), ads.RankSelect.restore_mapped(p),
              pickle.loads(pickle.dumps(rs))):
        assert r.rank1(5000) == rs.rank1(5000) and r.ones == rs.ones


@pytest.mark.parametrize("bits", [1, 2, 4, 8, 16, 32])
def test_packed_arrays(bits, tmp_path):
    v = rng.integers(0, 2 ** bits, 1000, dtype=np.uint64)
    p = ads.packed_array(v, bits)
    assert len(p) == 1000 and p.bits == bits
    assert np.array_equal(p.to_numpy(), v)
    assert np.array_equal(p.get(np.array([0, 999], dtype=np.uint64)),
                          v[[0, 999]])
    assert p.nbytes == 8 * len(p.words)
    q = type(p).from_words(p.words, len(p))
    assert np.array_equal(q.to_numpy(), v)
    p[3] = 1
    assert p[3] == 1
    with pytest.raises(ValueError):
        p[0] = 2 ** bits
    f = tmp_path / "p.ads"
    p.store(f)
    assert np.array_equal(type(p).restore_mapped(f).to_numpy(), p.to_numpy())


def test_elias_fano(tmp_path):
    v = np.sort(rng.integers(0, 10 ** 9, 5000, dtype=np.uint64))
    e = ads.EliasFano(v)
    assert len(e) == len(v) and e.universe == v[-1] + 1
    assert np.array_equal(e.to_numpy(), v)
    i = np.arange(len(v), dtype=np.uint64)
    assert np.array_equal(e.access(i), v)
    x = rng.integers(0, 10 ** 9, 1000, dtype=np.uint64)
    assert np.array_equal(e.next_geq(x), np.searchsorted(v, x, "left"))
    assert int(v[17]) in e
    assert e.size_in_bits < 64 * len(v) / 2
    with pytest.raises(ValueError):
        ads.EliasFano(v[::-1].copy())
    f = tmp_path / "e.ads"
    e.store(f)
    assert np.array_equal(ads.EliasFano.restore_mapped(f).to_numpy(), v)


def test_roaring_portable_format():
    v = np.unique(np.concatenate([
        rng.integers(0, 2 ** 32, 3000, dtype=np.uint64),
        np.arange(70000, 140000), np.arange(1 << 20, (1 << 20) + 9000, 3)
    ])).astype(np.uint32)
    r = ads.RoaringBitmap(v[::-1].copy())
    assert len(r) == len(v) and np.array_equal(r.to_numpy(), v)
    assert r.contains(v[:10]).all() and int(v[5]) in r
    s = r.serialize()
    assert ads.RoaringBitmap.deserialize(s) == r
    assert pickle.loads(pickle.dumps(r)) == r
    pyroaring = pytest.importorskip("pyroaring")
    b = pyroaring.BitMap.deserialize(s)
    assert len(b) == len(v) and np.array_equal(np.array(b, dtype=np.uint32), v)
    b.add(7)
    b.run_optimize()
    back = ads.RoaringBitmap.deserialize(b.serialize())
    assert len(back) == len(v) + (7 not in r) and 7 in back
    o = ads.RoaringBitmap(np.arange(0, 100000, 7, dtype=np.uint32))
    w = np.arange(0, 100000, 7)
    assert np.array_equal((r & o).to_numpy(), np.intersect1d(v, w))
    assert np.array_equal((r | o).to_numpy(), np.union1d(v, w))
    assert np.array_equal((r - o).to_numpy(), np.setdiff1d(v, w))
    assert np.array_equal((r ^ o).to_numpy(), np.setxor1d(v, w))


FILTERS = [
    lambda k: ads.BloomFilter.for_items(len(k), 0.01),
    lambda k: ads.BlockedBloomFilter(len(k)),
    lambda k: ads.CuckooFilter(len(k)),
    lambda k: ads.PrefixFilter(len(k)),
]
STATIC = [ads.BinaryFuseFilter8, ads.BinaryFuseFilter16, ads.RibbonFilter,
          ads.GrafiteFilter]


@pytest.mark.parametrize("make", FILTERS)
def test_dynamic_filters(make, tmp_path):
    keys = rng.integers(0, 2 ** 63, 20000, dtype=np.uint64)
    f = make(keys)
    assert f.update(keys) == len(keys)
    assert f.contains(keys).all() and int(keys[0]) in f
    others = rng.integers(0, 2 ** 63, 20000, dtype=np.uint64)
    assert f.contains(others).mean() < 0.06
    p = tmp_path / "f.ads"
    f.store(p)
    g = type(f).restore_mapped(p)
    assert np.array_equal(g.contains(others), f.contains(others))
    assert pickle.loads(pickle.dumps(f)).contains(keys).all()


@pytest.mark.parametrize("cls", STATIC)
def test_static_filters(cls, tmp_path):
    keys = rng.integers(0, 2 ** 40, 20000, dtype=np.uint64)
    f = cls(keys)
    assert f.contains(keys).all()
    others = rng.integers(0, 2 ** 40, 20000, dtype=np.uint64)
    assert f.contains(others).mean() < 0.05
    p = tmp_path / "f.ads"
    f.store(p)
    assert cls.restore(p).contains(keys).all()
    if cls is ads.GrafiteFilter:
        k = int(np.sort(keys)[100])
        assert f.may_contain_range(k - 3, k + 3)


def test_cuckoo_deletions():
    f = ads.CuckooFilter(1000)
    assert f.add(42) and 42 in f
    assert f.discard(42) and 42 not in f


def test_indexed_heaps():
    keys = rng.random(500)
    h = ads.IndexedHeap(500)
    h.push_many(np.arange(500, dtype=np.int32), keys)
    for i in range(0, 500, 5):
        keys[i] /= 2
        h.decrease(i, keys[i])
    for i in range(1, 500, 7):
        h.erase(i)
    ref = [(k, i) for i, k in enumerate(keys) if i % 7 != 1]
    heapq.heapify(ref)
    while h:
        i, k = h.pop()
        assert (k, i) == heapq.heappop(ref)
    assert not ref
    with pytest.raises(IndexError):
        h.pop()
    with pytest.raises(KeyError):
        h.decrease(3, 0.0)
    b = ads.IndexedBinaryHeap(4)
    assert b.push_or_decrease(2, 3.0) and not b.push_or_decrease(2, 4.0)
    with pytest.raises(ValueError):
        b.decrease(2, 9.0)
    p = ads.IndexedHeapPair(10)
    for i in range(10):
        p.push(i, float(10 - i), float(i))
    assert p.top() == (9, 1.0) and p.min_bound() == (0, 0.0)
    assert p.pop() == 9 and p.pop_min_bound() == 0 and len(p) == 8
