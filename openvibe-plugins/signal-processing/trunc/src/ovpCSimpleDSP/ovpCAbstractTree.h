#ifndef __OpenViBEPlugins_SignalProcessing_CAbstractTree_H__
#define __OpenViBEPlugins_SignalProcessing_CAbstractTree_H__

#include <openvibe-toolkit/ovtk_all.h>

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <boost/spirit/core.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <boost/spirit/tree/ast.hpp>
#include <boost/spirit/tree/tree_to_xml.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "ovpCEquationParserGrammar.h"
#include "ovpCEquationParser.h"

class CEquationParser;


/**
* Abstract class for an AST tree node
*
*/
class CAbstractTreeNode
{
	protected:

		//! True if this is a terminal node
		OpenViBE::boolean m_bIsTerminal;
		//! True if this node contains a constant value
		OpenViBE::boolean m_bIsConstant;


	public:
		CAbstractTreeNode(const OpenViBE::boolean bTerminal, const OpenViBE::boolean bIsConstant)
			:m_bIsTerminal(bTerminal)
			,m_bIsConstant(bIsConstant)
		{
		}

		//! virtual destructor
		virtual ~CAbstractTreeNode(void)
		{
		}

		/**
		* Used to know if this node is a leaf.
		* \return True if the node is a leaf.
		*/
		virtual OpenViBE::boolean isTerminal() const
		{
			return m_bIsTerminal;
		}

		/**
		 * Used to know if this node is a constant value node.
		 * \return True if the node is a constant value node.
		 */
		virtual OpenViBE::boolean isConstant() const
		{
			return m_bIsConstant;
		}

		//! Prints the node to stdout.
		virtual void print() = 0 ;

		/**
		* Used to simplify this node (and its children if any).
		* \param pModifiedNode Reference to a pointer to modify if the
		* current node object is to be destroyed and replaced. This pointer
		* will contain the address of the new node.
		*/
		virtual OpenViBE::boolean simplify(CAbstractTreeNode *& pModifiedNode) = 0;

		/**
		* Part of the process of simplification.
		* Levels recursively the associative operators nodes.
		*/
		virtual void levelOperators() = 0;

		/**
		* Changes the tree so it uses the NEG operator whenever it is possible.
		* (ie replaces (* -1 X) by (NEG X)
		*/
		virtual void useNegationOperator() = 0;

		/**
		* Generates the set of function calls needed to do the desired computation.
		* \param oParser The parser containing the function pointers stack and function contexts stack.
		*/
		virtual void generateCode(CEquationParser& oParser) = 0;
};


/**
* A tree's parent node (has children).
*
*/
class CAbstractTreeParentNode : public CAbstractTreeNode
{
	public:
		//! Children of this node
		std::vector<CAbstractTreeNode *> m_oChildren;

		//! The node operator's identifier
		OpenViBE::uint64 m_ui64Identifier;

		//! True if the node is "associative"
		OpenViBE::boolean m_bIsAssociative;

	public:
		//Constructors
		CAbstractTreeParentNode(OpenViBE::uint64 ui64NodeIdentifier, OpenViBE::boolean bIsAssociative = false) : CAbstractTreeNode(false, false), m_ui64Identifier(ui64NodeIdentifier), m_bIsAssociative(bIsAssociative){}

		CAbstractTreeParentNode(OpenViBE::uint64 ui64NodeIdentifier, CAbstractTreeNode* pChild, OpenViBE::boolean bIsAssociative = false)
			:CAbstractTreeNode(false, false)
			,m_ui64Identifier(ui64NodeIdentifier)
			,m_bIsAssociative(bIsAssociative)
		{
			m_oChildren.push_back(pChild);
		}

		CAbstractTreeParentNode(OpenViBE::uint64 ui64NodeIdentifier, CAbstractTreeNode* pLeftChild, CAbstractTreeNode* pRightChild, OpenViBE::boolean bIsAssociative = false)
			:CAbstractTreeNode(false, false)
			,m_ui64Identifier(ui64NodeIdentifier)
			,m_bIsAssociative(bIsAssociative)
		{
			m_oChildren.push_back(pLeftChild);
			m_oChildren.push_back(pRightChild);
		}

		/**
		 * Returns the node's operator identifier.
		 * \return The operator identifier
		 */
		OpenViBE::uint64 getOperatorIdentifier() { return m_ui64Identifier; }

		/**
		 * Used to know if the node is an associative node.
		 * \return True if the node is an associative one.
		 */
		OpenViBE::boolean isAssociative() { return m_bIsAssociative; }

		/**
		 * Returns the vector of children of the node.
		 * \return A reference to the vector of children.
		 */
		virtual std::vector<CAbstractTreeNode *>& getChildren() { return m_oChildren; }

		/**
		 * Adds a child to this node.
		 * \param pChild The child to add.
		 */
		virtual void addChild(CAbstractTreeNode * pChild) { m_oChildren.push_back(pChild); }

		//! Destructor.
		virtual ~CAbstractTreeParentNode(void);

		//! Debug function, prints the node and its children (prefix notation)
		virtual void print()
		{
			std::string op;
			switch(m_ui64Identifier)
			{
				case OP_ADD:
					op = "+";
					break;

				case OP_SUB:
					op = "-";
					break;

				case OP_MUL:
					op = "*";
					break;

				case OP_DIV:
					op = "/";
					break;

				case OP_NEG:
					op = "NEG";
					break;

				case OP_EXP:
					op = "exp";
					break;

				default:
					op = "unknownOp";
					break;
			}

			std::cout<<"("<<op<<" ";

			for(size_t i=0 ; i<m_oChildren.size() ; i++)
			{
				if(m_oChildren[i] == NULL)
				{
				}
				else
				{
					m_oChildren[i] -> print();
				}
				std::cout<<" ";
			}
			std::cout<<")";
		}

		virtual OpenViBE::boolean simplify(CAbstractTreeNode *& pModifiedNode);
		virtual void levelOperators();
		virtual void useNegationOperator();
		virtual void generateCode(CEquationParser& oParser);
};

/**
 * Class for terminal nodes containing a single value.
 *
 */
class CAbstractTreeValueNode : public CAbstractTreeNode
{
	//! Value associated with the node.
	OpenViBE::float64	m_f64Value;

	public:

		CAbstractTreeValueNode(OpenViBE::float64 f64Value) : CAbstractTreeNode(true, true), m_f64Value(f64Value) {}

		//! Destructor
		virtual ~CAbstractTreeValueNode()
		{
		}

		/**
		* Used to set the value of the node.
		* \param f64NewValue The node's new value.
		*/
		void setValue(OpenViBE::float64 f64NewValue) { m_f64Value=f64NewValue; }

		/**
		 * Used to know the value of the node.
		 * \return The node's value.
		 */
		OpenViBE::float64 getValue() { return m_f64Value; }

		virtual void print(){ std::cout<<m_f64Value; }

		virtual OpenViBE::boolean simplify(CAbstractTreeNode *& pModifiedNode)
		{
			pModifiedNode = this;
			return false;
		}

		virtual void levelOperators(){}
		virtual void useNegationOperator(){}
		virtual void generateCode(CEquationParser& oParser);
};

/**
 * Class for terminal nodes referencing a variable.
 */
class CAbstractTreeVariableNode : public CAbstractTreeNode
{
	public:
		CAbstractTreeVariableNode() : CAbstractTreeNode(true, false) {}

		virtual ~CAbstractTreeVariableNode()
		{
		}

		virtual void print(){ std::cout<<"X"; }
		virtual OpenViBE::boolean simplify(CAbstractTreeNode *& pModifiedNode)
		{
			pModifiedNode = this;
			return false;
		}

		virtual void levelOperators(){}
		virtual void useNegationOperator(){}
		virtual void generateCode(CEquationParser& oParser);

};

/**
* Main class for the AST.
* Contains the root of the tree.
*/
class CAbstractTree
{
	//! the root of the AST tree.
	CAbstractTreeNode * m_pRoot;

	public:
		//! Constructor
		CAbstractTree(CAbstractTreeNode * pRoot) : m_pRoot(pRoot) {}
		//! Destructor
		~CAbstractTree() { delete m_pRoot; }

		//! Prints the whole tree.
		void printTree() { m_pRoot -> print(); }

		/**
		 * Used to simplify the tree.
		 */
		void simplifyTree();

		/**
		 * Part of the process of simplification.
		 * Levels recursively the associative operators nodes.
		 */
		void levelOperators();

		/**
		 * Changes the tree so it uses the NEG operator whenever it is possible.
		 * (ie replaces (* -1 X) by (NEG X)
		 */
		void useNegationOperator(){ m_pRoot->useNegationOperator(); }

		/**
		 * Generates the set of function calls needed to do the desired computation.
		 * \param oParser The parser containing the function pointers stack and function contexts stack.
		 */
		void generateCode(CEquationParser& oParser);

		/**
		* Tries to recognize simple tree structures (X*X, X*Cste, X+Cste,...)
		* \param ui64TreeIdentifier The identifier of the tree (OP_USERDEF for non special tree).
		* \param f64Parameter The optional parameter if it is a special tree.
		*/
		void recognizeSpecialTree(OpenViBE::uint64& ui64TreeIdentifier, OpenViBE::float64& f64Parameter);
};




/**
* Functor used to compare two nodes.
* The order is as follow : Constants, Variables, ParentNodes
*/
struct CAbstractTreeNodeOrderingFunction
{
	bool operator() (CAbstractTreeNode * const & pFirstNode, CAbstractTreeNode * const & pSecondNode) const
	{
#if 0
		if( (pFirstNode->isConstant()) ||
			(pFirstNode->isTerminal() && !pSecondNode->isConstant()) ||
			(!pFirstNode->isTerminal() && !pSecondNode->isTerminal()))
		{
			return true;
		}
		else
		{
			return false;
		}
#else

	// Check isConstant flag
	if(pFirstNode->isConstant() && !pSecondNode->isConstant()) { return true; }
	if(!pFirstNode->isConstant() && pSecondNode->isConstant()) { return false; }

	// Check isTerminal flag
	if(pFirstNode->isTerminal() && !pSecondNode->isTerminal()) { return true; }
	if(!pFirstNode->isTerminal() && pSecondNode->isTerminal()) { return false; }

	// At this point, isTerminal and isConstant are the same for both value
	// Order is not important any more, we just compare the pointer values
	// so to have strict ordering function
	return pFirstNode < pSecondNode;
#endif
	}
};



#endif
