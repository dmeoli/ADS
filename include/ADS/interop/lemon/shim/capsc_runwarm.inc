 /// \brief Run the algorithm again from the last run.
 ///
 /// After a change of the costs, of the upper bounds or of the supplies,
 /// the flow and the potentials of the last run are the start rather than
 /// the zero ones, which a primal-dual algorithm allows after any such
 /// change: the flow of each arc is put within its new bounds, and then at
 /// the bound its reduced cost under the potentials says, so that every
 /// residual arc has a nonnegative reduced cost again; what this leaves at
 /// the nodes is sent by the shortest augmenting paths, as in a run from
 /// scratch. It does what run() does if the last run did not end with
 /// OPTIMAL, with lower bounds, with an arc of infinite upper bound and with
 /// supplies that do not sum to zero.
 ProblemType runWarm( int factor = 4 ) {
  if( ! _warm || _node_num == 1 || _has_lower || _sum_supply != 0 )
   return( run( factor ) );
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
   return( run( factor ) );
  for( int j = 0 ; j != _res_arc_num ; ++j ) {
   if( _forward[ j ] && _source[ j ] != _root && _upper[ j ] >= MAX )
    return( run( factor ) );
   }
  _factor = factor;
  _unit = std::numeric_limits< Value >::is_exact
	    ? Value( 1 )
	    : ( feps > 0 ? feps : ( std::numeric_limits< Value >::min )() );
  _ceps = _cost_tol >= 0
	    ? _cost_tol
	    : _mcf_bits::costTolerance( _root , _first_out , _target ,
					_forward , _cost );
  for( int i = 0 ; i != _root ; ++i )
   _excess[ i ] = _supply[ i ];
  _excess[ _root ] = 0;

  // the potentials of the last run, shifted so that the smallest is zero,
  // which leaves the reduced costs as they are and keeps the values from
  // drifting over a long sequence of runs
  Cost pmin = _pi[ 0 ];
  for( int i = 1 ; i <= _root ; ++i ) {
   if( _pi[ i ] < pmin )
    pmin = _pi[ i ];
   }
  for( int i = 0 ; i <= _root ; ++i )
   _pi[ i ] -= pmin;

  // the flow of the last run, within the new bounds and at the bound that
  // the reduced cost says; the last arc out of a node is the one to the
  // root
  for( int u = 0 ; u != _root ; ++u ) {
   const int last_out = _first_out[ u + 1 ] - 1;
   for( int j = _first_out[ u ] ; j != last_out ; ++j ) {
    if( ! _forward[ j ] )
     continue;
    const int v = _target[ j ] , rj = _reverse[ j ];
    const Value up = _upper[ j ];
    Value x = _res_cap[ rj ];
    if( x > up )
     x = up;
    const Cost c = _cost[ j ] + _pi[ u ] - _pi[ v ];
    if( c < -_ceps )
     x = up;
    else if( c > _ceps )
     x = 0;
    _res_cap[ j ] = up - x;
    _res_cap[ rj ] = x;
    _excess[ u ] -= x;
    _excess[ v ] += x;
    }
   }
  for( int a = _first_out[ _root ] ; a != _res_arc_num ; ++a ) {
   _res_cap[ a ] = 1;
   _res_cap[ _reverse[ a ] ] = 0;
   }

  // the scaling starts from the largest excess, as in init()
  if( _factor > 1 ) {
   Value max_sup = 0 , max_dem = 0 , max_cap = 0;
   for( int i = 0 ; i != _root ; ++i ) {
    const Value ex = _excess[ i ];
    if( ex > max_sup )
     max_sup = ex;
    if( -ex > max_dem )
     max_dem = -ex;
    const int last_out = _first_out[ i + 1 ] - 1;
    for( int j = _first_out[ i ] ; j != last_out ; ++j ) {
     if( _res_cap[ j ] > max_cap )
      max_cap = _res_cap[ j ];
     }
    }
   max_sup = ( std::min )( ( std::min )( max_sup , max_dem ) , max_cap );
   for( _delta = _unit ; 2 * _delta <= max_sup ; _delta *= 2 )
    ;
   }
  else
   _delta = _unit;
  return( remember( start() ) );
  }

 private:
 // the flow and the potentials of the last run are there for runWarm()
 bool _warm = false;

 public:
