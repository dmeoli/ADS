 /// \brief Initialize the algorithm from a flow that may have deficits.
 ///
 /// The flow of each arc is put within its capacity, and the deficit that
 /// this or the flow given leave at a node other than the source, more
 /// flow out of it than into it, is moved forward: the flow of the arcs out
 /// of the node is lowered by the deficit, which goes to their heads, until
 /// no node but the source has one. Each step lowers the flow of an arc, so
 /// that with integer values the repair ends after at most the total flow
 /// of steps, and it is the excess left at the tails that the phases of the
 /// algorithm send on, as init( flowMap ) does. Unlike init( flowMap ),
 /// which refuses such a flow, this always gives a preflow to start from.
 /// \pre The digraph has the nodes it had when the algorithm was built.
 template< typename FlowMap >
 void initWarm( const FlowMap & flowMap ) {
  createStructures();
  for( ArcIt e( _graph ) ; e != INVALID ; ++e ) {
   Value f = flowMap[ e ];
   if( _tolerance.negative( f ) )
    f = 0;
   if( _tolerance.less( ( *_capacity )[ e ] , f ) )
    f = ( *_capacity )[ e ];
   _flow->set( e , f );
   }
  repairDeficits();
  const bool ok = init( *_flow );
  LEMON_ASSERT( ok , "A deficit is left after the repair" );
  ( void )ok;
  }

 /// \brief Run the algorithm again from the flow of the last run.
 ///
 /// After a change of the capacities, the flow of the last run is the
 /// start rather than the zero flow [see initWarm()], and the two phases
 /// run from it. It does what run() does if there is no flow of a last run,
 /// or if the digraph has not the nodes it had when the algorithm was built.
 void runWarm() {
  if( ! warmReady() ) {
   run();
   return;
   }
  initWarm( *_flow );
  startFirstPhase();
  startSecondPhase();
  }

 /// \brief Whether runWarm() has the flow of a last run to start from,
 /// on the nodes that the algorithm was built with.
 bool warmReady() const {
  return( _flow != 0 && countNodes( _graph ) == _node_num );
  }

 private:
 // the deficits of the nodes other than the source moved forward along the
 // arcs that carry flow out of them [see initWarm()]
 void repairDeficits() {
  typename Digraph::template NodeMap< Value > ex( _graph , 0 );
  typename Digraph::template NodeMap< bool > queued( _graph , false );
  for( ArcIt e( _graph ) ; e != INVALID ; ++e ) {
   ex[ _graph.target( e ) ] += ( *_flow )[ e ];
   ex[ _graph.source( e ) ] -= ( *_flow )[ e ];
   }
  std::vector< Node > stack;
  for( NodeIt n( _graph ) ; n != INVALID ; ++n ) {
   if( n != _source && _tolerance.negative( ex[ n ] ) ) {
    stack.push_back( n );
    queued[ n ] = true;
    }
   }
  while( ! stack.empty() ) {
   const Node n = stack.back();
   stack.pop_back();
   queued[ n ] = false;
   for( OutArcIt e( _graph , n ) ;
	e != INVALID && _tolerance.negative( ex[ n ] ) ; ++e ) {
    const Value f = ( *_flow )[ e ];
    if( ! _tolerance.positive( f ) )
     continue;
    const Value d = _tolerance.less( f , -ex[ n ] ) ? f : -ex[ n ];
    _flow->set( e , f - d );
    ex[ n ] += d;
    const Node v = _graph.target( e );
    ex[ v ] -= d;
    if( v != _source && ! queued[ v ] && _tolerance.negative( ex[ v ] ) ) {
     stack.push_back( v );
     queued[ v ] = true;
     }
    }
   if( _tolerance.negative( ex[ n ] ) )  // within the tolerance, rounded
    ex[ n ] = 0;
   }
  }

 public:
