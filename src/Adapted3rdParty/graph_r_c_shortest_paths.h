/*
Adapted from <boost/graph/r_c_shortest_paths.hpp>

My contribution: using a dominance that returns -1/0/1 and not bool
And calling just once, not twice this 'function'.
The comparison is > and <, not >=, not <=

Normal use of the lib is through 'r_c_shortest_paths' calls
which all pass the params to 'r_c_shortest_paths_dispatch'.

My approach is to call directly a 'r_c_shortest_paths_dispatch_adapted'
with all the parameters specified
*/

#ifndef H_GRAPH_R_C_SHORTEST_PATHS
#define H_GRAPH_R_C_SHORTEST_PATHS

#pragma warning( push, 0 )

#include <boost/graph/r_c_shortest_paths.hpp>

#pragma warning( pop )

namespace boost {

	namespace detail {
		// r_c_shortest_paths_dispatch_adapted function (body/implementation)
		template<class Graph, 
		class VertexIndexMap, 
		class EdgeIndexMap, 
		class Resource_Container, 
		class Resource_Extension_Function, 
		class Dominance_Function, 
		class Label_Allocator, 
		class Visitor>
			void r_c_shortest_paths_dispatch_adapted
					( const Graph& g, 
					const VertexIndexMap& vertex_index_map, 
					const EdgeIndexMap& /*edge_index_map*/, 
					typename graph_traits<Graph>::vertex_descriptor s, 
					typename graph_traits<Graph>::vertex_descriptor t, 
					// each inner vector corresponds to a pareto-optimal path
					std::vector< std::vector< typename graph_traits< Graph >::edge_descriptor> >& pareto_optimal_solutions, 
					std::vector< Resource_Container >& pareto_optimal_resource_containers, 
					bool b_all_pareto_optimal_solutions, 
					// to initialize the first label/resource container 
					// and to carry the type information
					const Resource_Container& rc, 
					Resource_Extension_Function& ref, 
					Dominance_Function& dominance, 
					// to specify the memory management strategy for the labels
					Label_Allocator /*la*/, 
					Visitor vis )
		{
			typedef typename boost::graph_traits<Graph>::vertices_size_type  vertices_size_type;

			typedef typename Label_Allocator::template rebind< r_c_shortest_paths_label< Graph, Resource_Container > >::other LAlloc;
			typedef ks_smart_pointer< r_c_shortest_paths_label< Graph, Resource_Container > > Splabel;

			pareto_optimal_resource_containers.clear();
			pareto_optimal_solutions.clear();

			size_t i_label_num = 0;
			LAlloc l_alloc;
			std::priority_queue< Splabel, std::vector< Splabel >, std::greater< Splabel > >   unprocessed_labels;

			bool b_feasible = true;
			r_c_shortest_paths_label<Graph, Resource_Container>* first_label = l_alloc.allocate( 1 );
			l_alloc.construct(first_label, r_c_shortest_paths_label< Graph, Resource_Container >(
									(unsigned long)i_label_num++, rc, 0, typename graph_traits<Graph>::edge_descriptor(), s ) );

			Splabel splabel_first_label = Splabel( first_label );
			unprocessed_labels.push( splabel_first_label );
			std::vector<std::list<Splabel> > vec_vertex_labels( num_vertices( g ) );
			vec_vertex_labels[size_t(vertex_index_map[size_t(s)])].push_back( splabel_first_label );
			std::vector<typename std::list<Splabel>::iterator>  vec_last_valid_positions_for_dominance( num_vertices( g ) );
			for( vertices_size_type i = 0; i < num_vertices( g ); ++i )
				vec_last_valid_positions_for_dominance[i] = vec_vertex_labels[i].begin();
			std::vector<size_t> vec_last_valid_index_for_dominance( num_vertices( g ), 0 );
			std::vector<bool> b_vec_vertex_already_checked_for_dominance( num_vertices( g ), false );
			
			while( !unprocessed_labels.empty()  && vis.on_enter_loop(unprocessed_labels, g) ) {
				Splabel cur_label = unprocessed_labels.top();
				unprocessed_labels.pop();
				vis.on_label_popped( *cur_label, g );
				// an Splabel object in unprocessed_labels and the respective Splabel 
				// object in the respective list<Splabel> of vec_vertex_labels share their 
				// embedded r_c_shortest_paths_label object
				// to avoid memory leaks, dominated 
				// r_c_shortest_paths_label objects are marked and deleted when popped 
				// from unprocessed_labels, as they can no longer be deleted at the end of 
				// the function; only the Splabel object in unprocessed_labels still 
				// references the r_c_shortest_paths_label object
				// this is also for efficiency, because the else branch is executed only 
				// if there is a chance that extending the 
				// label leads to new undominated labels, which in turn is possible only 
				// if the label to be extended is undominated
				if( !cur_label->b_is_dominated ) { // If some other label that dominates cur_label was processed earlier
					vertices_size_type i_cur_resident_vertex_num = (vertices_size_type)get(vertex_index_map, cur_label->resident_vertex);
					
					// the present list of labels associated with the current vertex
					std::list<Splabel>& list_labels_cur_vertex = vec_vertex_labels[i_cur_resident_vertex_num];
					if( list_labels_cur_vertex.size() >= 2 
							&& vec_last_valid_index_for_dominance[i_cur_resident_vertex_num] < list_labels_cur_vertex.size() ) {
						
						typename std::list<Splabel>::iterator outer_iter = list_labels_cur_vertex.begin();
						bool b_outer_iter_at_or_beyond_last_valid_pos_for_dominance = false;
						
						// iterate through the current labels of this vertex
						while( outer_iter != list_labels_cur_vertex.end() ) {
							Splabel cur_outer_splabel = *outer_iter;
							typename std::list<Splabel>::iterator inner_iter = outer_iter;
							if( !b_outer_iter_at_or_beyond_last_valid_pos_for_dominance 
									&& outer_iter == vec_last_valid_positions_for_dominance[i_cur_resident_vertex_num] )
								b_outer_iter_at_or_beyond_last_valid_pos_for_dominance = true;
							
							if( !b_vec_vertex_already_checked_for_dominance[i_cur_resident_vertex_num] 
									|| b_outer_iter_at_or_beyond_last_valid_pos_for_dominance ) {
								++inner_iter;
							
							} else {
								inner_iter = vec_last_valid_positions_for_dominance[i_cur_resident_vertex_num];
								++inner_iter;
							}

							bool b_outer_iter_erased = false;
							
							// iterate through the rest of the labels of this vertex
							while( inner_iter != list_labels_cur_vertex.end() ) {
								Splabel cur_inner_splabel = *inner_iter;
								
								// My contribution: using a dominance that returns -1/0/1 and not bool
								// And calling just once, not twice this 'function'.
								// The comparison is > and <, not >=, not <=
								int dominanceResult = dominance( cur_outer_splabel->cumulated_resource_consumption, 
																cur_inner_splabel->cumulated_resource_consumption );
								// is  cur_inner_splabel  dominated ?
								if( dominanceResult > 0 ) {
									typename std::list<Splabel>::iterator buf = inner_iter;
									++inner_iter;
									list_labels_cur_vertex.erase( buf );


									if( cur_inner_splabel->b_is_processed ) {
										l_alloc.destroy( cur_inner_splabel.get() );
										l_alloc.deallocate( cur_inner_splabel.get(), 1 );
									
									} else
										cur_inner_splabel->b_is_dominated = true;
									
									// skips the 2nd evaluation of domination from below with an invalid param
									continue;

								} else
									++inner_iter;

								// The other way around comparison:
								// is  cur_outer_splabel  dominated ?
								if( dominanceResult < 0 ) {
									typename std::list<Splabel>::iterator buf = outer_iter;
									++outer_iter;
									list_labels_cur_vertex.erase( buf );
									b_outer_iter_erased = true;

									if( cur_outer_splabel->b_is_processed ) {
										l_alloc.destroy( cur_outer_splabel.get() );
										l_alloc.deallocate( cur_outer_splabel.get(), 1 );

									} else
										cur_outer_splabel->b_is_dominated = true;
									
									// leaving the whole pairwise dominance comparing if cur_outer_splabel is dominated
									break;
								}
							}
							
							if( !b_outer_iter_erased )
								++outer_iter;
						}
						
						if( list_labels_cur_vertex.size() > 1 )
							vec_last_valid_positions_for_dominance[i_cur_resident_vertex_num] = 
																					(--(list_labels_cur_vertex.end()));
						else
							vec_last_valid_positions_for_dominance[i_cur_resident_vertex_num] = 
																					list_labels_cur_vertex.begin();
						
						b_vec_vertex_already_checked_for_dominance[i_cur_resident_vertex_num] = true;
						vec_last_valid_index_for_dominance[i_cur_resident_vertex_num] = 
																					list_labels_cur_vertex.size() - 1;
					}
				}
				
				// When requested to find the 1st solution and just found it:
				if( !b_all_pareto_optimal_solutions && cur_label->resident_vertex == t ) {
					l_alloc.destroy( cur_label.get() );
					l_alloc.deallocate( cur_label.get(), 1 );

					while( unprocessed_labels.size() ) {
						Splabel l = unprocessed_labels.top();
						unprocessed_labels.pop();
						
						// delete only dominated labels, because nondominated labels are 
						// deleted at the end of the function
						if( l->b_is_dominated ) {
							l_alloc.destroy( l.get() );
							l_alloc.deallocate( l.get(), 1 );
						}
					}

					break;
				}
				
				if( !cur_label->b_is_dominated ) {
					cur_label->b_is_processed = true;
					vis.on_label_not_dominated( *cur_label, g );
					
					typename graph_traits<Graph>::vertex_descriptor cur_vertex = cur_label->resident_vertex;
					typename graph_traits<Graph>::out_edge_iterator oei, oei_end;
					
					// expand from cur_vertex through all outgoing edges
					for( boost::tie( oei, oei_end ) = out_edges( cur_vertex, g ); oei != oei_end; ++oei ) {
						b_feasible = true;
						r_c_shortest_paths_label<Graph, Resource_Container>* new_label = l_alloc.allocate( 1 );
						l_alloc.construct( new_label, r_c_shortest_paths_label< Graph, Resource_Container >
															( (unsigned long)i_label_num++,  cur_label->cumulated_resource_consumption, 
															cur_label.get(), *oei, target( *oei, g ) ) );
						
						b_feasible = ref( g, new_label->cumulated_resource_consumption, 
							new_label->p_pred_label->cumulated_resource_consumption, new_label->pred_edge );

						if( !b_feasible ) {
							vis.on_label_not_feasible( *new_label, g );
						
							l_alloc.destroy( new_label );
							l_alloc.deallocate( new_label, 1 );
						
						} else { // b_feasible  is true
							const r_c_shortest_paths_label<Graph, Resource_Container> &ref_new_label = *new_label;
							vis.on_label_feasible( ref_new_label, g );
							
							Splabel new_sp_label( new_label );
							vec_vertex_labels[size_t(vertex_index_map[size_t(new_sp_label->resident_vertex)])].push_back( new_sp_label );
							unprocessed_labels.push( new_sp_label );
						}
					}
				
				} else { // cur_label->b_is_dominated  is true
					vis.on_label_dominated( *cur_label, g );

					l_alloc.destroy( cur_label.get() );
					l_alloc.deallocate( cur_label.get(), 1 );
				}
			}
			
			std::list<Splabel> dsplabels = vec_vertex_labels[size_t(vertex_index_map[size_t(t)])];
			typename std::list<Splabel>::const_iterator csi = dsplabels.begin();
			typename std::list<Splabel>::const_iterator csi_end = dsplabels.end();
			// if d could be reached from o
			if( !dsplabels.empty() ) {
				for( ; csi != csi_end; ++csi ) {
					std::vector< typename graph_traits< Graph >::edge_descriptor >  cur_pareto_optimal_path;
					const r_c_shortest_paths_label< Graph, Resource_Container >* p_cur_label =  (*csi).get();
					pareto_optimal_resource_containers.push_back( p_cur_label->cumulated_resource_consumption );
					
					while( p_cur_label->num != 0 ) {
						cur_pareto_optimal_path.push_back( p_cur_label->pred_edge );
						p_cur_label = p_cur_label->p_pred_label;
					}

					pareto_optimal_solutions.push_back( cur_pareto_optimal_path );
					
					if( !b_all_pareto_optimal_solutions )
						break;
				}
			}

			size_t i_size = vec_vertex_labels.size();
			for( size_t i = 0; i < i_size; ++i ) {
				const std::list<Splabel>& list_labels_cur_vertex = vec_vertex_labels[i];
				csi_end = list_labels_cur_vertex.end();
				
				for( csi = list_labels_cur_vertex.begin(); csi != csi_end; ++csi ) {
					l_alloc.destroy( (*csi).get() );
					l_alloc.deallocate( (*csi).get(), 1 );
				}
			}
		} // r_c_shortest_paths_dispatch_adapted

	} // detail

} // namespace

#endif // H_GRAPH_R_C_SHORTEST_PATHS
