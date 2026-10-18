/*--------------------------------------------------------------------------*/
/*------------------------- File ExpressionTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree of an arithmetic expression (the parse tree), beyond the book: a
 * binary tree [see BinaryTree.h] whose leaves are the operands, numbers or
 * names of variables, and whose internal nodes are the binary operators
 * + - * / ^ (the power), the left subtree being the left operand.
 *
 * - expressionTree() builds it from the postfix form of the expression
 *   [see Expressions.h] with a stack of trees: an operand is pushed as a
 *   leaf, an operator pops its right and then its left subtree and pushes
 *   the new tree, in O( n );
 * - evaluate() computes its value by a postvisit, the values of the
 *   variables given by a dictionary;
 * - prefixForm() writes it in prefix form, each subtree in parentheses,
 *   e.g., (*(/(3)(+(*(^(x)(2))(y))(*(3)(x))))(-(z)(1))) for
 *   ( 3 / ( x^2 y + 3 x ) ) ( z - 1 ), and infixForm() in infix form, fully
 *   parenthesized, by an invisit.
 *
 * The contracts [see Contract.h]: the postfix expression is well formed,
 * i.e., each operator finds two subtrees and one tree is left; evaluate()
 * needs a value for each variable, and no division by 0. The functions are
 * defined in ExpressionTree.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.5 and 5.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ExpressionTree
 #define __ADS_ExpressionTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/tree/BinaryTree.h"

#include <cmath>
#include <map>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the expression tree of the postfix expression e, whose tokens are
/// separated by spaces; the caller owns it
/// \pre e is a well formed postfix expression
BinaryTree< std::string > * expressionTree( std::string_view e );

/// the value of the expression t, with the variables valued by env; a
/// leaf is a number, possibly signed (e.g., -3, +2.5, .5), or a variable
/// \pre t is not nil, env values each variable, no division by 0
double evaluate( const BinaryTree< std::string > * t ,
                 const std::map< std::string , double > & env = {} );

/// the prefix form of t, each subtree in parentheses
/// \pre t is not nil
std::string prefixForm( const BinaryTree< std::string > * t );

/// the infix form of t, fully parenthesized
/// \pre t is not nil
std::string infixForm( const BinaryTree< std::string > * t );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ExpressionTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ExpressionTree.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ExpressionTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
