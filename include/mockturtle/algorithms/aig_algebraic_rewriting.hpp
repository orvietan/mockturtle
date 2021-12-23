/*!
  \file aig_algebraic_rewriting.hpp
  \brief AIG algebraric rewriting

  EPFL CS-472 2021 Final Project Option 1
*/

#pragma once

#include "../networks/aig.hpp"
#include "../views/depth_view.hpp"
#include "../views/topo_view.hpp"


namespace mockturtle
{

namespace detail
{

template<class Ntk>
class aig_algebraic_rewriting_impl
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  aig_algebraic_rewriting_impl( Ntk& ntk )
      : ntk( ntk )
  {
    static_assert( has_level_v<Ntk>, "Ntk does not implement depth interface." );
  }

  void run()
  {
    bool cont{ true }; /* continue trying */
    while ( cont )
    {
      cont = false; /* break the loop if no updates can be made */
      ntk.foreach_gate( [&]( node n )
                        {
                          if ( try_algebraic_rules( n ) )
                          {
                            ntk.update_levels();
                            cont = true;
                          }
                        } );
    }
  }

private:
  /* Try various algebraic rules on node n. Return true if the network is updated. */
  bool try_algebraic_rules( node n )
  {
    if ( try_associativity( n ) )
      return true;
    if ( try_distributivity( n ) )
      return true;
    /* TODO: add more rules here... */
    // Additional boolean property optimizable
    if ( try_three_layer_distributivity( n ) )
      return true;
     
    return false;
  }

  /* Try the associativity rule on node n. Return true if the network is updated. */
  bool try_associativity( node n )
  {
    /* TODO */
    /* if a name of a variable presents CP = critical path  */

    // Flag used to adfirm if the associativity is feasible or not
    bool flag_associativity = false;

    // Flag to assert if the rule can be applied to the n 
    bool is_feas = false;

    // Assert the level of n
    uint32_t current_level = 0;
    // Assert the level of the children of n for a given fanin
    uint32_t current_level_child = 0;


    signal signal_child_non_CP, signal_non_CP, signal_child_CP;

    // Counter for the number of child on critical path
    uint32_t CNT_child = 0;

    // Level of the node in the non-critical path and level of the granchildren node in the critical path 
    uint32_t level_child_signal_non_CP, level_nephew_signal_CP;

    if ( ntk.is_on_critical_path( n ) )
    {
      // children of node n -- if current level = 1 -> means connected to PI's
      current_level = ntk.level( n );

      // PI's equal to 0 --> gate with fanin's PI's has level 1
      if ( current_level != 1 ) 
      {
        ntk.foreach_fanin( n, [&]( signal const& signal_in_n )
        {
           node n_child = ntk.get_node( signal_in_n );
         
           // check if it is not a PI
           if ( ntk.is_on_critical_path( n_child ) )
           {
               // If either the first or the second fanin has a non complemented signal the associativity is applicable
                if ( !ntk.is_complemented( signal_in_n ) )
                {
                 
                 current_level_child = ntk.level( n_child );
                  // PI's equal to 0 --> gate with fanin's PI's has level 1
                  if ( current_level_child != 1 )
                  {
                     ntk.foreach_fanin( n_child, [&]( signal const& signal_in_n_child )
                     {

                        node n_2nd_child = ntk.get_node( signal_in_n_child );
                        if (ntk.is_on_critical_path( n_2nd_child ) )
                        {
                          CNT_child++;
                          signal_child_CP = signal_in_n_child;
                          level_nephew_signal_CP = ntk.level( n_2nd_child );
                        }
                        else    
                          signal_child_non_CP = signal_in_n_child;
                        return;
                     });
                  }
                  
                  is_feas = true;
                }
           }
           else
           {
             signal_non_CP = signal_in_n;
             level_child_signal_non_CP = ntk.level( n_child );
           }
        return;
        });
      }
    }

    // Whenever the is_feas is true and the number of children of n is more than 2
    if ( is_feas && CNT_child == 1 )
    {
    // uint32_t level_child_signal_non_CP =  ntk.get_node( signal_non_CP ) , level_nephew_signal_CP = ntk.get_node( signal_child_CP ) ;
    // uint32_t level_child_signal_non_CP =ntk.level(ntk.get_node( signal_non_CP )), level_nephew_signal_CP = ntk.level(ntk.get_node( signal_child_CP ));

    // Verification if the associatovoty is favorable or not and the substitution can be avoided
     if ( ( level_child_signal_non_CP <= level_nephew_signal_CP - 1 ) )
      {
        signal new_and_1th = ntk.create_and( signal_non_CP, signal_child_non_CP );
        ntk.substitute_node( n, ntk.create_and( new_and_1th, signal_child_CP ) );
        flag_associativity = true;
      }
    }

    return flag_associativity;
  }

  /* Try the distributivity rule on node n. Return true if the network is updated. */
  bool try_distributivity( node n )
  {
    /* TODO */
    /* if a name of a variable presents CP = critical path  */

    // Flag used to adfirm if the distributivity is feasible or not
    bool flag_distributivity = false;
    bool is_feas = false, is_feas_child = false, is_feas_child1 = false;
    // bool is_PI;
    uint32_t current_level, current_level_child;
  
    // Children of n
    std::vector<node> nodes;
    // Fanin of children of n (CP or not CP)
    std::vector<signal> signal_child_CP, signal_child_non_CP;

    if ( ntk.is_on_critical_path( n ) )
    {
      ntk.foreach_fanin( n, [&]( signal const& signal_in_n )
                       {
                       // Save the children of n only if the fanin is complemented: in AIG 
                       // an OR is an AND with inverting input
                           if(ntk.is_complemented(signal_in_n))
                           {
                             node child = ntk.get_node( signal_in_n );
                             if (ntk.is_on_critical_path(child))
                               nodes.push_back(child);
                           }
                        return;
                        });

      // Verify the both the children are on critical path
      if (nodes.size() == ntk.fanin_size(n))
      { 
         
          for (auto ii = 0; ii < nodes.size(); ii++)
          {
            ntk.foreach_fanin( nodes.at( ii ), [&]( signal const& signal_in_n )
                               { 
                                 if ( ntk.is_on_critical_path( ntk.get_node( signal_in_n ) ) )
                                   signal_child_CP.push_back( signal_in_n );
                                 else
                                   signal_child_non_CP.push_back( signal_in_n );  
                                 return; 
                               });
          }
          // Check if the fanin's of each childre are not empty
          if ( !signal_child_CP.empty() && !signal_child_non_CP.empty() )
          {
              // Check the presence of shared fanin for apllying distributivity --> only favorable if they are on critical path
            if ( ( signal_child_CP.at( 0 ) == signal_child_CP.at( 1 ) ))
            {
              flag_distributivity = true;
              signal new_or = ntk.create_or( signal_child_non_CP.at( 0 ), signal_child_non_CP.at( 1 ) );
              signal new_logic;
              // if n is an or the output of n is not complemented --> creation of an and, otherwise creation of an nand
              if ( ntk.is_or( n ) )
                new_logic = ntk.create_and( signal_child_CP.at( 0 ), new_or );
              else
                new_logic = ntk.create_nand( signal_child_CP.at( 0 ), new_or );
                ntk.substitute_node( n, new_logic );
            }
         
          }

      }

    }

    return flag_distributivity;
  }

  bool try_three_layer_distributivity( node n )
  {
    // Flag used to adfirm if the three layer distributivity is feasible or not
    bool flag_three_layer_distributivity = false;
 
    node n_child;

    // Basically accordings sequence of condition to verify if the structure is composed of three layer of nand 
    if ( ntk.is_on_critical_path( n ) )
    {
      three_layer_help( n ); 

      if ( children_on_three_layer.size()== 1 && children_other.size()== 1 )
     
          three_layer_help( ntk.get_node(children_on_three_layer.at( 0 ) ));       
        
      if ( children_on_three_layer.size() == 2 && children_other.size() == 2 )
      {
          ntk.foreach_fanin( ntk.get_node(children_on_three_layer.at( 1 )), [&]( signal const& signal_in_n )
                             {
                               n_child = ntk.get_node( signal_in_n );
                               if ( ntk.is_on_critical_path( n_child ) )
                                 children_on_three_layer.push_back( signal_in_n );
                               else 
                                 children_other.push_back( signal_in_n );
                               return;
                             });
      }

      // Replace the overall structure
      if ( children_on_three_layer.size() == 3 && children_other.size() == 3 )
      {
        
        uint32_t level_three_layer = ntk.level( ntk.get_node(children_on_three_layer.at( 0 )) ), level_opt = ntk.level( ntk.get_node(children_other.at( 0 ) ));
        
        // Check  if it is favorable the optimization or not
        if ( level_three_layer - 2 > level_opt )
        {
          signal new_and_1st = ntk.create_and( children_other.at( 2 ), children_other.at( 0 ) );
          signal new_and_2nd = ntk.create_and( children_on_three_layer.at( 2 ), new_and_1st );
          signal new_and3rd = ntk.create_and( children_other.at( 0 ), !children_other.at( 1 ) );
          signal new_logic = ntk.create_nand( !new_and_2nd, !new_and3rd );

          ntk.substitute_node( n, new_logic);
          flag_three_layer_distributivity = true;
        }
        
      } 

    }

    children_on_three_layer.clear();
    children_other.clear();

    return flag_three_layer_distributivity;
  }

  void three_layer_help(node const& n) {

       ntk.foreach_fanin( n, [&]( signal const& signal_in_n )
                       {
                         node n_child = ntk.get_node( signal_in_n );
                         if ( ntk.is_on_critical_path( n_child ) && ntk.is_complemented( signal_in_n ) )
                           children_on_three_layer.push_back( signal_in_n );
                         else if ( !ntk.is_on_critical_path( n_child ) )
                           children_other.push_back( signal_in_n );
                         return;
                       } );
      return;
  }

private:
  Ntk& ntk;
  std::vector<signal> children_on_three_layer, children_other;
};



} // namespace detail

/* Entry point for users to call */
template<class Ntk>
void aig_algebraic_rewriting( Ntk& ntk )
{
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk is not an AIG" );

  depth_view dntk{ ntk };
  detail::aig_algebraic_rewriting_impl p( dntk );
  p.run();
}

} /* namespace mockturtle */
