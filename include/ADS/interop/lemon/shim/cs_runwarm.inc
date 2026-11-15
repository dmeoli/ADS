 /// \brief Run the algorithm again from the last run.
 ///
 /// After a change of the costs, of the upper bounds or of the supplies,
 /// the flow and the potentials of the last run are the start rather than
 /// a feasible flow found anew and the zero potentials: the flow is put
 /// within the new bounds, what this and a change of the supplies leave at
 /// the nodes is sent where it is due on the residual network, and the
 /// scaling starts from the largest amount by which a residual arc violates
 /// the optimality conditions under the potentials. It does what run()
 /// does if the last run did not end with OPTIMAL, with lower bounds, with
 /// an arc of infinite upper bound, with supplies that do not sum to zero,
 /// when no flow meets the new bounds and supplies, and when the potentials
 /// times the scaling do not fit in an integer LargeCost or violate the
 /// optimality conditions by more than the largest cost.
 ProblemType runWarm( Method method = PARTIAL_AUGMENT , int factor = 16 ) {
  if( ! _warm || _node_num == 0 || _has_lower || _sum_supply != 0 ||
      ! _inf_upper.empty() )
   return( run( method , factor ) );
  LEMON_ASSERT( factor >= 2 , "The scaling factor must be at least 2" );
  const Value MAX = ( std::numeric_limits< Value >::max )();
  const Value feps =
    _flow_tol >= 0
      ? _flow_tol
      : _mcf_bits::flowTolerance( _root , _first_out , _target , _forward ,
				  _lower , _upper , _supply , _has_lower );
  // the potentials of the last run, times n alpha, and the reduced costs
  // they give must fit in an integer LargeCost, which after a change of the
  // costs they need not do
  const LargeCost scale = static_cast< LargeCost >( _res_node_num ) * factor;
  if( std::numeric_limits< LargeCost >::is_integer ) {
   const LargeCost lim = ( std::numeric_limits< LargeCost >::max )() / 4
			 / scale;
   for( int i = 0 ; i != _res_node_num ; ++i ) {
    if( _pi[ i ] > lim || _pi[ i ] < -lim )
     return( run( method , factor ) );
    }
   }
  Value sum = 0;
  for( int i = 0 ; i != _root ; ++i )
   sum += _supply[ i ];
  if( sum > feps || sum < -feps )
   return( run( method , factor ) );
  for( int j = 0 ; j != _res_arc_num ; ++j ) {
   if( _forward[ j ] && _source[ j ] != _root && _upper[ j ] >= MAX )
    return( run( method , factor ) );
   }
  _feps = feps;
  _warm_flow = true;
  const bool feasible =
    _mcf_bits::feasibleFlow( *this , Value( 0 ) , _sup_node_num , _feps ,
				  _cut_hint );
  _warm_flow = false;
  if( ! feasible )
   return( run( method , factor ) );

  _alpha = factor;
  _ceps = _cost_tol >= 0
	    ? _cost_tol
	    : _mcf_bits::costTolerance( _root , _first_out , _target ,
					_forward , _scost );
  _eps_min = std::numeric_limits< LargeCost >::is_integer
	       ? LargeCost( 1 )
	       : static_cast< LargeCost >( _ceps ) * scale;
  for( int i = 0 ; i != _res_node_num ; ++i ) {
   _excess[ i ] = 0;
   _pi[ i ] *= scale;
   }

  // the costs the algorithm works on, and how far the potentials are from
  // optimal
  _epsilon = 0;
  LargeCost cold = 0;  // the epsilon times alpha of run()
  for( int j = 0 ; j != _res_arc_num ; ++j ) {
   if( _source[ j ] == _root || _target[ j ] == _root )
    continue;
   _cost[ j ] = static_cast< LargeCost >( _scost[ j ] ) * scale;
   if( _cost[ j ] > cold )
    cold = _cost[ j ];
   }
  for( int j = 0 ; j != _res_arc_num ; ++j ) {
   if( _res_cap[ j ] <= _feps )
    continue;
   const LargeCost rc = _cost[ j ] + _pi[ _source[ j ] ] - _pi[ _target[ j ] ];
   if( -rc > _epsilon )
    _epsilon = -rc;
   }

  // potentials that violate the optimality conditions by more than the
  // costs do are a worse start than the zero ones of run(), and with an
  // integer LargeCost the phases from such an epsilon overflow it
  if( _epsilon > cold )
   return( run( method , factor ) );

  // the flow and the potentials are already epsilon-optimal: the first
  // phase is the one after epsilon, as the loop of start() would take it
  _epsilon = _epsilon >= _alpha * _eps_min
	       ? _epsilon / _alpha
	       : ( _epsilon >= _eps_min ? _eps_min : _epsilon );

  _max_rank = _alpha * _res_node_num;
  _buckets.resize( _max_rank );
  start( method );
  return( remember( OPTIMAL ) );
  }

 private:
 // the flow and the potentials of the last run are there for runWarm(),
 // and feasibleFlow() starts from that flow while _warm_flow
 bool _warm = false , _warm_flow = false;

 public:
