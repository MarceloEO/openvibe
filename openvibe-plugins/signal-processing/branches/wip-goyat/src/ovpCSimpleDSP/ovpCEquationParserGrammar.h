#ifndef __OpenViBEPlugins_SignalProcessing_CEquationParserGrammar_H__
#define __OpenViBEPlugins_SignalProcessing_CEquationParserGrammar_H__

#include <openvibe-toolkit/ovtk_all.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <boost/spirit/core.hpp>
#include <boost/spirit/symbols/symbols.hpp>
#include <boost/spirit/tree/ast.hpp>


using namespace boost::spirit;

/**
* Enum of parent nodes identifiers.
*/
enum ByteCodes
{
	OP_NEG,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,

	OP_ABS,
	OP_ACOS,
	OP_ASIN,
	OP_ATAN,
	OP_CEIL,
	OP_COS,
	OP_EXP,
	OP_FLOOR,
	OP_LOG,
	OP_LOG10,
	OP_POW,
	OP_SIN,
	OP_SQRT,
	OP_TAN,

	//used for special tree recognition

	//The equation is not a special one
	OP_USERDEF,
	//Identity
	OP_NONE,
	//X*X
	OP_X2
};



/**
* Symbols table for binary functions.
*
*/
struct CBinaryFunctionSymbols : symbols<OpenViBE::uint64>
{
	CBinaryFunctionSymbols()
	{
		add
				("pow"  , OP_POW)
				;
	}

} ;

/**
* Symbols table for unary functions.
*
*/
struct CUnaryFunctionSymbols : symbols<OpenViBE::uint64>
{
	CUnaryFunctionSymbols()
	{
		add
				("abs"   , OP_ABS)
				("acos"  , OP_ACOS)
				("asin"  , OP_ASIN)
				("atan"  , OP_ATAN)
				("ceil"  , OP_CEIL)
				("cos"   , OP_COS)
				("exp"   , OP_EXP)
				("floor" , OP_FLOOR)
				("log"   , OP_LOG)
				("log10" , OP_LOG10)
				("sin"   , OP_SIN)
				("sqrt"  , OP_SQRT)
				("tan"   , OP_TAN)
				;
	}

} ;

/**
* Symbols table for mathematical constants.
*
*/
struct CMathConstantSymbols : symbols<OpenViBE::float64>
{
	CMathConstantSymbols()
	{
		add
				("m_pi"         , 3.14159265358979323846)
				("m_pi_2"       , 1.57079632679489661923)
				("m_pi_4"       , 0.78539816339744830962)
				("m_1_pi"       , 0.31830988618379067154)
				("m_2_pi"       , 0.63661977236758134308)
				("m_2_sqrt"   , 1.12837916709551257390)
				("m_sqrt2"      , 1.41421356237309504880)
				("m_sqrt1_2"    , 0.70710678118654752440)
				("m_e"          , 2.7182818284590452354)
				("m_log2e"      , 1.4426950408889634074)
				("m_log10e"     , 0.43429448190325182765)
				("m_ln"        , 0.69314718055994530942)
				("m_ln10"       , 2.30258509299404568402)
				;
	}

} ;



static CUnaryFunctionSymbols unaryFunction_p;
static CBinaryFunctionSymbols binaryFunction_p;
static CMathConstantSymbols mathConstant_p;

/**
* The parser's grammar.
*/
struct CEquationGrammar : public grammar<CEquationGrammar>
{
	static const int realID = 1;
	static const int variableID = 2;
	static const int functionID = 3;
	static const int constantID = 4;
	static const int factorID = 6;
	static const int termID = 7;
	static const int expressionID = 8;


    template <typename ScannerT>
    struct definition
    {
        definition(CEquationGrammar const&)
        {
            real =
			    leaf_node_d[real_p]
                ;

	    variable =
			   leaf_node_d[as_lower_d[ch_p('x')]]
		;

	    constant = leaf_node_d[as_lower_d[mathConstant_p]]
			    ;

	    function = (
			    root_node_d[as_lower_d[unaryFunction_p]]>>no_node_d[ch_p('(')]>>expression
			    | root_node_d[as_lower_d[binaryFunction_p]]>>no_node_d[ch_p('(')]>> infix_node_d[(expression>>','>>expression)]
		       ) >> no_node_d[ch_p(')')];


            factor =
			    (real | variable | constant | function)
			    |   inner_node_d['(' >> expression >> ')']
			    |   (root_node_d[ch_p('-')] >> factor)
			    |   (root_node_d[ch_p('+')] >> factor)
                ;

            term =
                factor
			    >> *(   (root_node_d[ch_p('*')] >> factor)
			    	|   (root_node_d[ch_p('/')] >> factor)
				)
                    ;

            expression =
                term
			    >> *(   (root_node_d[ch_p('+')] >> term)
			    	|   (root_node_d[ch_p('-')] >> term)
                    )
                    ;

        }


	rule<ScannerT, parser_context<>, parser_tag<expressionID> > expression;
	rule<ScannerT, parser_context<>, parser_tag<termID> > term;
	rule<ScannerT, parser_context<>, parser_tag<factorID> > factor;
	rule<ScannerT, parser_context<>, parser_tag<realID> > real;
	rule<ScannerT, parser_context<>, parser_tag<variableID> > variable;
	rule<ScannerT, parser_context<>, parser_tag<functionID> > function;
	rule<ScannerT, parser_context<>, parser_tag<constantID> > constant;

	rule<ScannerT, parser_context<>, parser_tag<expressionID> > const& start() const { return expression; }
    };

};

#endif