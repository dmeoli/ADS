 /// \brief Run the algorithm again from the flow of the last run.
 ///
 /// After a change of the costs, of the upper bounds or of the supplies,
 /// the flow of the last run is the start rather than a feasible flow
 /// found anew: it is put within the new bounds, what this and a change of
 /// the supplies leave at the nodes is sent where it is due on the residual
 /// network, and the negative cycles are canceled from there. It does what
 /// run() does if the last run did not end with OPTIMAL, with lower bounds,
 /// with an arc of infinite upper bound, with supplies that do not sum to
 /// zero, and when no flow meets the new bounds and supplies.
 ProblemType runWarm( Method method = CANCEL_AND_TIGHTEN ) {
  if( ! _warm || _node_num == 0 || _has_lower || _sum_supply != 0 ||
      ! _inf_upper.empty() )
   return( run( method ) );
  const Value MAX = ( std::numeric_limits< Value >::max )();
  const Value feps =
    _flow_tol >= 0
      ? _flow_tol
      : _mcf_bits::flowTolerance( _root , _first_out , _target , _forward ,
				  _lower , _upper , _supply , _has_lower );
  Value sum = 0;
  for( int i = 0 ; i != _root ; ++i )
   sum += _supply[ i ];
  if( sum > feps || sum < -feps )
   return( run( method ) );
  for( int j = 0 ; j != _res_arc_num ; ++j ) {
   if( _forward[ j ] && _source[ j ] != _root && _upper[ j ] >= MAX )
    return( run( method ) );
   }
  _feps = feps;
  int sup_num;
  _warm_flow = true;
  const bool feasible =
    _mcf_bits::feasibleFlow( *this , Value( 1 ) , sup_num , _feps ,
				  _cut_hint );
  _warm_flow = false;
  if( ! feasible )
   return( run( method ) );
  _ceps = _cost_tol >= 0
	    ? _cost_tol
	    : _mcf_bits::costTolerance( _root , _first_out , _target ,
					_forward , _cost );

  // the zero potentials that init() starts from
  for( int i = 0 ; i != _res_node_num ; ++i )
   _pi[ i ] = 0;
  start( method );
  return( remember( OPTIMAL ) );
  }

 private:
 // the flow of the last run is there for runWarm(), and feasibleFlow()
 // starts from it while _warm_flow
 bool _warm = false , _warm_flow = false;

 public:
