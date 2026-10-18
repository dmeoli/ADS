/*--------------------------------------------------------------------------*/
/*--------------------------- File Expressions.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Arithmetic expressions with a STACK [see ADT.h], beyond the book: the
 * operands are nonnegative integers, the operators the binary + - * /
 * (the division truncated toward zero), and the tokens of an expression may
 * be separated by spaces.
 *
 * - evalPostfix() evaluates an expression in postfix (reverse Polish)
 *   notation, e.g., 5 9 8 + 4 6 * * 7 + *: an operand is pushed, an
 *   operator pops its right operand t and then its left operand s and
 *   pushes s op t, and the value is the one item left, in O( n );
 * - toPostfixParenthesized() turns a fully parenthesized infix expression,
 *   e.g., ( 5 * ( ( ( 9 + 8 ) * ( 4 * 6 ) ) + 7 ) ), into postfix: an
 *   operand goes to the output, an open parenthesis is skipped, an operator
 *   is pushed, and a closed parenthesis pops one operator to the output;
 * - toPostfix() does the same for an infix expression with the usual
 *   precedences (* and / before + and -) and the associativity to the left,
 *   e.g., 35 + 17 * ( 40 - 9 ) - 7, by the shunting-yard algorithm of
 *   Dijkstra: an operator first pops to the output the operators on the
 *   stack, down to an open parenthesis, whose precedence is not lower than
 *   its own, and a closed parenthesis pops them down to the open one,
 *   which it removes.
 *
 * The contracts [see Contract.h]: each operator of a postfix expression
 * finds two operands on the stack, one item is left at the end, and no
 * division is by 0; an infix expression has balanced parentheses and only
 * the tokens above, and a fully parenthesized one has an operator for each
 * pair of parentheses. The functions are defined in Expressions.tpp.
 *
 * \see J. Lukasiewicz, Elementy logiki matematycznej, Warszawa, 1929
 * \see E. W. Dijkstra, Algol 60 translation, Report MR 34/61, Mathematisch
 *      Centrum, Amsterdam, 1961
 * \see R. Sedgewick, Algorithms in C++, Parts 1-4, 3rd ed., Addison-Wesley,
 *      1998, section 4.3
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

#ifndef __ADS_Expressions
 #define __ADS_Expressions
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/stack/ArrayStack.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the value of the postfix expression e
/// \pre e is a well formed postfix expression with no division by 0
long evalPostfix( std::string_view e );

/// the postfix form of the fully parenthesized infix expression e, its
/// tokens separated by one space
/// \pre e is fully parenthesized
std::string toPostfixParenthesized( std::string_view e );

/// the postfix form of the infix expression e, with the usual precedences
/// and the associativity to the left, its tokens separated by one space
/// \pre the parentheses of e are balanced
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
