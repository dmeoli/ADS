 /// \brief Run the algorithm again from the basis of the last run.
 ///
 /// After a change of the costs alone the basis is still primal feasible:
 /// its potentials are computed anew and the simplex goes on from there.
 /// After a change of the upper bounds or of the supplies the flow of its
 /// tree is computed anew, and the subtree below an arc whose flow falls
 /// outside its bounds is hung from the root by an artificial arc, so that
 /// the basis is primal feasible again [see repairBasis()]. A basis is kept
 /// from a run that ended with OPTIMAL or, without lower bounds, with
 /// INFEASIBLE; a change of the digraph, of the lower bounds, of the supply
 /// type, stSupply(), resetParams(), reset(), or supplies that do not sum
 /// to zero drop it, and then this does what run() does.
 ProblemType runWarm( PivotRule pivot_rule = BLOCK_SEARCH ) {
  if( ! _warm || _node_num == 0 )
   return( run( pivot_rule ) );
  _unb_arc = -1;
  if( _repair ) {
   if( ! repairBasis() )
    return( run( pivot_rule ) );
   _repair = false;
   }

  // the flow and the supplies in the form start() works on, the lower
  // bounds taken out of them as init() does
  if( _has_lower ) {
   for( int i = 0 ; i != _arc_num ; ++i ) {
    const Value c = _lower[ i ];
    _flow[ i ] -= c;
    _supply[ _source[ i ] ] -= c;
    _supply[ _target[ i ] ] += c;
    }
   _supply_shifted = true;
   }

  // the artificial arcs cost more than any path, which the change may
  // have made costlier
  const Cost art = initArtificialCost();
  for( int e = _arc_num ; e != _all_arc_num ; ++e ) {
   if( _cost[ e ] != 0 )
    _cost[ e ] = art;
   }

  // the potentials of the tree, in the order of the thread, which gives
  // every node after its father
  _pi[ _root ] = 0;
  for( int u = _thread[ _root ] ; u != _root ; u = _thread[ u ] ) {
   const int e = _pred[ u ];
   _pi[ u ] = _pred_dir[ u ] == DIR_UP ? _pi[ _parent[ u ] ] - _cost[ e ]
				       : _pi[ _parent[ u ] ] + _cost[ e ];
   }
  const ProblemType pt =
    finish( feasibleUnbounded( start( pivot_rule ) , pivot_rule ) );
  _result = pt;
  return( pt );
  }

 private:
 // the basis of the last run is there for runWarm(), which has to make it
 // primal feasible first [see repairBasis()]
 bool _warm = false , _repair = false;

 // the new upper bound of the arc i, the basis kept as it is if the flow
 // of the arc stays within it, the arc does not sit at its upper bound and
 // a tree arc keeps the tree strongly feasible; otherwise runWarm() has to
 // recompute the flow of the tree; with lower bounds the basis is dropped
 void warmUpper( int i , Value up ) {
  if( up == _upper[ i ] )
   return;
  _upper[ i ] = up;
  if( ! _warm )
   return;
  Value cap;
  if( _has_lower ) {
   const Value c = _lower[ i ];
   if( c >= 0 )
    cap = up < MAX_VALUE ? up - c : INF;
   else
    cap = up < MAX_VALUE + c ? up - c : INF;
   }
  else
   cap = up;

  bool moves = _state[ i ] == STATE_UPPER || _flow[ i ] > cap;
  if( ! moves && _state[ i ] == STATE_TREE && _flow[ i ] >= cap ) {
   const int u = _pred[ _source[ i ] ] == i ? _source[ i ] : _target[ i ];
   moves = _pred_dir[ u ] == DIR_UP;
   }
  if( moves && _has_lower ) {
   _warm = false;
   return;
   }
  _cap[ i ] = cap;
  if( moves )
   _repair = true;
  }

 // the new supply of the node i, the flow of the tree recomputed by
 // runWarm()
 void warmSupply( int i , Value s ) {
  if( s == _supply[ i ] )
   return;
  _supply[ i ] = s;
  if( _warm )
   _repair = true;
  }

 // the basis of the last run made primal feasible again after a change of
 // the upper bounds or of the supplies, its tree kept: every arc out of the
 // tree is put at the bound it sits at, and the flow of the tree arcs is
 // recomputed from the leaves up; an arc whose flow falls outside its
 // bounds, or sits at the bound that would leave the tree not strongly
 // feasible, leaves the tree at that bound, and the subtree below it hangs
 // from the root by the artificial arc of its top node, directed as the
 // flow it has to carry and with the cost init() gives it, so that the
 // simplex drives it out; the thread and the sizes of the subtrees are
 // rebuilt from the fathers. False, the basis to be dropped, with lower
 // bounds or supplies that do not sum to zero, for which the artificial
 // arcs are built otherwise.
 bool repairBasis() {
  if( _has_lower || _sum_supply != 0 )
   return( false );
  Value sum = 0;
  for( int u = 0 ; u != _node_num ; ++u )
   sum += _supply[ u ];
  const Value feps = flowTol();
  if( sum > feps || sum < -feps )
   return( false );

  std::vector< Value > net( _node_num + 1 , 0 );
  for( int u = 0 ; u != _node_num ; ++u )
   net[ u ] = _supply[ u ];
  for( int e = 0 ; e != _arc_num ; ++e ) {
   if( _state[ e ] == STATE_TREE )
    continue;
   if( _state[ e ] == STATE_UPPER && _cap[ e ] >= MAX_VALUE )
    _state[ e ] = STATE_LOWER;
   _flow[ e ] = _state[ e ] == STATE_UPPER ? _cap[ e ] : 0;
   net[ _source[ e ] ] -= _flow[ e ];
   net[ _target[ e ] ] += _flow[ e ];
   }
  for( int e = _arc_num ; e != _all_arc_num ; ++e ) {
   if( _state[ e ] != STATE_TREE )
    _flow[ e ] = 0;
   }

  // in the reverse order of the thread, which gives every node after its
  // subtree
  for( int u = _rev_thread[ _root ] ; u != _root ; u = _rev_thread[ u ] ) {
   int e = _pred[ u ];
   Value n = net[ u ];
   if( e < _arc_num ) {
    const int p = _parent[ u ];
    const Value f = _pred_dir[ u ] == DIR_UP ? n : -n;
    if( _pred_dir[ u ] == DIR_UP ? f >= 0 && f < _cap[ e ]
				 : f > 0 && f <= _cap[ e ] ) {
     _flow[ e ] = f;
     net[ p ] += n;
     continue;
     }
    _state[ e ] = f <= 0 ? STATE_LOWER : STATE_UPPER;
    _flow[ e ] = f <= 0 ? 0 : _cap[ e ];
    const Value s = _pred_dir[ u ] == DIR_UP ? _flow[ e ] : -_flow[ e ];
    n -= s;
    net[ p ] += s;
    e = _arc_num + u;
    _state[ e ] = STATE_TREE;
    _parent[ u ] = _root;
    _pred[ u ] = e;
    }
   if( n >= 0 ) {
    _pred_dir[ u ] = DIR_UP;
    _source[ e ] = u;
    _target[ e ] = _root;
    _flow[ e ] = n;
    _cost[ e ] = 0;
    }
   else {
    _pred_dir[ u ] = DIR_DOWN;
    _source[ e ] = _root;
    _target[ e ] = u;
    _flow[ e ] = -n;
    _cost[ e ] = 1;  // nonzero: runWarm() gives it the artificial cost
    }
   net[ _root ] += n;
   }

  std::vector< int > child( _node_num + 1 , -1 ) ,
    sibling( _node_num + 1 , -1 );
  for( int u = _node_num - 1 ; u >= 0 ; --u ) {
   sibling[ u ] = child[ _parent[ u ] ];
   child[ _parent[ u ] ] = u;
   }
  std::vector< int > order , stack( 1 , _root );
  order.reserve( _node_num + 1 );
  while( ! stack.empty() ) {
   const int u = stack.back();
   stack.pop_back();
   order.push_back( u );
   for( int v = child[ u ] ; v != -1 ; v = sibling[ v ] )
    stack.push_back( v );
   }
  const int k = static_cast< int >( order.size() );
  if( k != _node_num + 1 )
   return( false );
  for( int i = 0 ; i != k ; ++i ) {
   const int u = order[ i ] , v = order[ ( i + 1 ) % k ];
   _thread[ u ] = v;
   _rev_thread[ v ] = u;
   _succ_num[ u ] = 1;
   }
  for( int i = k - 1 ; i > 0 ; --i )
   _succ_num[ _parent[ order[ i ] ] ] += _succ_num[ order[ i ] ];
  for( int i = 0 ; i != k ; ++i )
   _last_succ[ order[ i ] ] = order[ i + _succ_num[ order[ i ] ] - 1 ];
  return( true );
  }

 // a basis is there to reuse after a run that ended at an optimal one, or
 // at one that proves the problem infeasible, which is optimal for the
 // problem with the artificial arcs; not with lower bounds, whose flow
 // start() puts back in the original form only when it is optimal
 ProblemType finish( ProblemType pt ) {
  _warm = pt == OPTIMAL || ( pt == INFEASIBLE && ! _has_lower );
  return( pt );
  }

 public:
