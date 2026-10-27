/*
 * The code that Doxygen appends to its configuration of MathJax 3 [see
 * MATHJAX_CODEFILE in docs/Doxyfile]: the formulae are typeset when they
 * come into view, since a theory page has hundreds of them, as on the
 * Sphinx site [see mathjax3_config in docs/conf.py].
 */
window.MathJax.loader.load.push( 'ui/lazy' );
