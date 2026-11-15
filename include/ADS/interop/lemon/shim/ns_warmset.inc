 /// \brief Set the cost of one arc for runWarm().
 ///
 /// The same as costMap() with a map that differs from the last one in
 /// the arc \c a alone, in constant time instead of a scan of the arcs.
 NetworkSimplex & warmCost( const Arc & a , const Cost & c ) {
  _cost[ _arc_id[ a ] ] = c;
  return( *this );
  }

 /// \brief Set the upper bound of one arc for runWarm().
 ///
 /// The same as upperMap() with a map that differs from the last one in
 /// the arc \c a alone, in constant time; the basis is marked to be
 /// repaired if the flow of the arc no longer fits [see repairBasis()].
 NetworkSimplex & warmUpper( const Arc & a , const Value & u ) {
  warmUpper( _arc_id[ a ] , u );
  return( *this );
  }

 /// \brief Set the supply of one node for runWarm().
 ///
 /// The same as supplyMap() with a map that differs from the last one in
 /// the node \c n alone, in constant time; the basis is marked to be
 /// repaired.
 NetworkSimplex & warmSupply( const Node & n , const Value & s ) {
  warmSupply( _node_id[ n ] , s );
  return( *this );
  }

 /// \brief Whether runWarm() has a basis or a flow of the last run to
 /// start from, which a change of the lower bounds, of the supply type, of
 /// the digraph or of all the parameters drops.
 bool warmReady() const { return( _warm ); }

