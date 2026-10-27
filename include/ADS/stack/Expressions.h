/*--------------------------------------------------------------------------*/
/*--------------------------- File Expressions.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Arithmetic expressions with a STACK [see `ADT.h`], beyond the book: the
 * operands are nonnegative integers, the operators the binary `+`, `-`, `*`
 * and `/` (the division truncated toward zero), and the tokens of an
 * expression may be separated by spaces.
 *
 * - `evalPostfix()` evaluates an expression in postfix (reverse Polish)
 *   notation, e.g., `5 9 8 + 4 6 * * 7 + *`: an operand is pushed, an
 *   operator pops its right operand \f$ t \f$ and then its left operand
 *   \f$ s \f$ and pushes \f$ s \mathbin{\mathrm{op}} t \f$, and the value is
 *   the one item left, in \f$ O( n ) \f$;
 * - `toPostfixParenthesized()` turns a fully parenthesized infix
 *   expression, e.g., `( 5 * ( ( ( 9 + 8 ) * ( 4 * 6 ) ) + 7 ) )`, into
 *   postfix: an operand goes to the output, an open parenthesis is skipped,
 *   an operator is pushed, and a closed parenthesis pops one operator to the
 *   output;
 * - `toPostfix()` does the same for an infix expression with the usual
 *   precedences (`*` and `/` before `+` and `-`) and the associativity to
 *   the left, e.g., `35 + 17 * ( 40 - 9 ) - 7`, by the shunting-yard
 *   algorithm of Dijkstra: an operator first pops to the output the
 *   operators on the stack, down to an open parenthesis, whose precedence is
 *   not lower than its own, and a closed parenthesis pops them down to the
 *   open one, which it removes.
 *
 * The contracts [see `Contract.h`]: each operator of a postfix expression
 * finds two operands on the stack, one item is left at the end, no division
 * is by 0 and no operation overflows a `long` (an operand beyond a `long`
 * throws the `std::out_of_range` of `std::stol`); an infix expression has
 * balanced parentheses and only the tokens above, and a fully parenthesized
 * one has an operator for each pair of parentheses. The functions are
 * defined in `Expressions.tpp`.
 *
 * \see J. Łukasiewicz, Elementy logiki matematycznej, Warszawa, 1929
 * \see K. Samelson, F. L. Bauer, Sequential formula translation,
 *      Communications of the ACM 3(2), 76-83, 1960,
 *      doi:10.1145/366959.366968
 * \see E. W. Dijkstra, Algol 60 translation, Report MR 34/61, Mathematisch
 *      Centrum, Amsterdam, 1961
 * \see R. Sedgewick, Algorithms in C++, Parts 1-4, 3rd ed., Addison-Wesley,
 *      1998, section 4.3
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

#ifndef __ADS_Expressions
 #define __ADS_Expressions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/ArrayStack.h"

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the value of the postfix expression \f$ e \f$
///
/// In pseudocode (Łukasiewicz, beyond the book), each operator replacing its
/// two operands on the stack with its result:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{evalPostfix}( e ) \cr
/// 2 & \quad S \gets \text{an empty stack} \cr
/// 3 & \quad \textbf{foreach}\ \text{token } x \text{ of } e\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \text{ is an operand}\ \textbf{then}
///     \ \mathrm{push}( S , x ) \cr
/// 5 & \qquad \textbf{else}\ t \gets \mathrm{pop}( S );\ s \gets \mathrm{pop}(
///     S );\ \mathrm{push}( S , s \mathbin{x} t ) \cr
/// 6 & \quad \textbf{return}\ \mathrm{pop}( S )
/// \end{array}}
/// \f]
/// \pre \f$ e \f$ is a well formed postfix expression with no division by
///      0 and no operation whose result is out of a `long`
long evalPostfix( std::string_view e );

/// the postfix form of the fully parenthesized infix expression \f$ e \f$,
/// its tokens separated by one space
///
/// In pseudocode (Sedgewick, section 4.3, beyond the book), each operator
/// written at the closed parenthesis of its pair:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{toPostfixParenthesized}( e ) \cr
/// 2 & \quad S \gets \text{an empty stack};\ \mathit{out} \gets
///     \text{the empty sequence} \cr
/// 3 & \quad \textbf{foreach}\ \text{token } x \text{ of } e\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \text{ is an operand}\ \textbf{then}
///     \ \text{append } x \text{ to } \mathit{out} \cr
/// 5 & \qquad \textbf{else if}\ x \text{ is an operator}\ \textbf{then}
///     \ \mathrm{push}( S , x ) \cr
/// 6 & \qquad \textbf{else if}\ x \text{ is a closed parenthesis}
///     \ \textbf{then}\ \text{append } \mathrm{pop}( S ) \text{ to }
///     \mathit{out} \cr
/// 7 & \quad \textbf{return}\ \mathit{out}
/// \end{array}}
/// \f]
/// \pre \f$ e \f$ is fully parenthesized
std::string toPostfixParenthesized( std::string_view e );

/// the postfix form of the infix expression \f$ e \f$, with the usual
/// precedences and the associativity to the left, its tokens separated by
/// one space
///
/// In pseudocode (the shunting-yard algorithm of Dijkstra, beyond the book),
/// \f$ \pi \f$ the precedence, 2 for `*` and `/` and 1 for `+` and `-`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{toPostfix}( e ) \cr
/// 2 & \quad S \gets \text{an empty stack};\ \mathit{out} \gets
///     \text{the empty sequence} \cr
/// 3 & \quad \textbf{foreach}\ \text{token } x \text{ of } e\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ x \text{ is an operand}\ \textbf{then}
///     \ \text{append } x \text{ to } \mathit{out} \cr
/// 5 & \qquad \textbf{else if}\ x \text{ is an open parenthesis}
///     \ \textbf{then}\ \mathrm{push}( S , x ) \cr
/// 6 & \qquad \textbf{else if}\ x \text{ is a closed parenthesis}
///     \ \textbf{then} \cr
/// 7 & \qquad\quad \textbf{while}\ \mathrm{top}( S ) \text{ is not an open }
///     \text{parenthesis}\ \textbf{do}\ \text{append } \mathrm{pop}( S )
///     \text{ to } \mathit{out} \cr
/// 8 & \qquad\quad \mathrm{pop}( S )\quad \triangleright\ \text{the open }
///     \text{parenthesis} \cr
/// 9 & \qquad \textbf{else} \cr
/// 10 & \qquad\quad \textbf{while}\ S \ne \emptyset\ \textbf{and}
///     \ \mathrm{top}( S ) \text{ is an operator}\ \textbf{and}\ \pi( x )
///     \le \pi( \mathrm{top}( S ) )\ \textbf{do} \cr
/// 11 & \qquad\qquad \text{append } \mathrm{pop}( S ) \text{ to }
///     \mathit{out} \cr
/// 12 & \qquad\quad \mathrm{push}( S , x ) \cr
/// 13 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do}
///     \ \text{append } \mathrm{pop}( S ) \text{ to } \mathit{out} \cr
/// 14 & \quad \textbf{return}\ \mathit{out}
/// \end{array}}
/// \f]
/// \pre the parentheses of \f$ e \f$ are balanced
std::string toPostfix( std::string_view e );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/Expressions.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Expressions.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Expressions.h -------------------------*/
/*--------------------------------------------------------------------------*/
