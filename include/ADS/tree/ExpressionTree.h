/*--------------------------------------------------------------------------*/
/*------------------------- File ExpressionTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree of an arithmetic expression (the parse tree), beyond the book: a
 * binary tree [see `BinaryTree.h`] whose leaves are the operands, numbers or
 * names of variables, and whose internal nodes are the binary operators `+`,
 * `-`, `*`, `/` and `^` (the power), the left subtree being the left
 * operand.
 *
 * - `expressionTree()` builds it from the postfix form of the expression
 *   [see `Expressions.h`] with a stack of trees: an operand is pushed as a
 *   leaf, an operator pops its right and then its left subtree and pushes
 *   the new tree, in \f$ O( n ) \f$;
 * - `evaluate()` computes its value by a postvisit, the values of the
 *   variables given by a dictionary;
 * - `prefixForm()` writes it in prefix form, each subtree in parentheses,
 *   e.g., `(*(/(3)(+(*(^(x)(2))(y))(*(3)(x))))(-(z)(1)))` for
 *   \f$ ( 3 / ( x^2 y + 3 x ) ) ( z - 1 ) \f$, and `infixForm()` in infix
 *   form, fully parenthesized, by an invisit.
 *
 * The contracts [see `Contract.h`]: the postfix expression is well formed,
 * i.e., each operator finds two subtrees and one tree is left; `evaluate()`
 * needs a value for each variable, and no division by 0. The functions are
 * defined in `ExpressionTree.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 5.5 and 5.6,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
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
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the expression tree of the postfix expression \f$ e \f$, whose tokens
/// are separated by spaces; the caller owns it
///
/// In pseudocode (sections 5.5 and 5.6, as `evalPostfix()` of
/// `Expressions.h` with trees in place of values):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{expressionTree}( e )\quad \triangleright
///     \ e \text{ in postfix form} \cr
/// 2 & \quad S \gets \text{an empty stack of trees} \cr
/// 3 & \quad \textbf{foreach}\ \text{token } x \text{ of } e
///     \text{, from the left}\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \text{ is an operator}\ \textbf{then}\ r \gets
///     \mathrm{pop}( S );\ l \gets \mathrm{pop}( S );\ \mathrm{push}( S ,
///     \mathrm{join}( x , l , r ) ) \cr
/// 5 & \qquad \textbf{else}\ \mathrm{push}( S , \text{the leaf } x ) \cr
/// 6 & \quad \textbf{return}\ \mathrm{pop}( S )\quad \triangleright
///     \ \text{the only tree left}
/// \end{array}}
/// \f]
/// \pre \f$ e \f$ is a well formed postfix expression
BinaryTree< std::string > * expressionTree( std::string_view e );

/// the value of the expression \f$ t \f$, with the variables valued by
/// `env`; a leaf is a number, possibly signed (e.g., `-3`, `+2.5`, `.5`),
/// or a variable
///
/// In pseudocode (section 5.6), a postvisit:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{evaluate}( t ) \cr
/// 2 & \quad \textbf{if}\ t \text{ is a leaf}\ \textbf{then return}
///     \ \text{the number in } t \text{, or the value of the variable in } t
///     \cr
/// 3 & \quad \textbf{return}\ \text{the operator of } t \text{ applied to }
///     \mathrm{evaluate}( t.\mathrm{left}() ) \text{ and }
///     \mathrm{evaluate}( t.\mathrm{right}() )
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is not nil, `env` values each variable, no division by
///      0
double evaluate( const BinaryTree< std::string > * t ,
		 const std::map< std::string , double > & env = {} );

/// the prefix form of \f$ t \f$, each subtree in parentheses, by a
/// previsit (section 5.6)
/// \pre \f$ t \f$ is not nil
std::string prefixForm( const BinaryTree< std::string > * t );

/// the infix form of \f$ t \f$, fully parenthesized, by an invisit
/// (section 5.6)
///
/// In pseudocode (section 5.6):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{infixForm}( t ) \cr
/// 2 & \quad \textbf{if}\ t \text{ is a leaf}\ \textbf{then return}
///     \ t.\mathrm{read}() \cr
/// 3 & \quad \textbf{return}\ \text{“(”} \cdot \mathrm{infixForm}(
///     t.\mathrm{left}() ) \cdot t.\mathrm{read}() \cdot \mathrm{infixForm}(
///     t.\mathrm{right}() ) \cdot \text{“)”}
/// \end{array}}
/// \f]
/// \pre \f$ t \f$ is not nil
std::string infixForm( const BinaryTree< std::string > * t );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/ExpressionTree.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ExpressionTree.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ExpressionTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
