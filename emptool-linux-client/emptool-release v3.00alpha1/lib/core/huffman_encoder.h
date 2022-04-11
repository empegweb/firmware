/* huffman_encoder.h
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.3 13-Mar-2003 18:15 rob:)
 */

#ifndef HUFFMAN_H
#define HUFFMAN_H	1

#include <list>
#include <map>
#include <set>

/* Incredibly inefficient generic Huffman Encoder.
 *
 * Please note that the maximum huffman code length will exceed 32
 * bits in extremely pathologically unlikely cases (e.g somebody
 * deliberately uses values with a fibonacci frequency distribution)
 * at about 14,000,000 data values. If you're going to use anything
 * near a million values, modify the code to automatically balance and
 * generate non-optimal sequences.
 *
 * This should work on non-POD types such as classes (or strings!) so
 * long as they implement "operator<" - it could work with just
 * "operator==" but you'd have to use something other than std::map.
 *
 * Usage: (slightly more long winded than the decoder)
 *
 *     HuffmanGenerator<unsigned> generator;
 *
 *     while(not_eof) // run all data through, this counts frequencies
 *         generator.Count(GetNextValue());
 *
 *     HuffmanEncoder<unsigned> encoder;
 *     generator.Encode(&encoder); // optimal encoder for those counts
 *
 *     while(not_eof) // same data set again
 *     {
 *         HuffmanEncoder<unsigned>::Code code = encoder.Get(GetNextValue());
 *         printf("Code %x bit length %u\n", code.code, code.bits);
 *     }
 *
 *     generator.Reset();
 *     encoder.Reset();
 *     ...
 */

template <class Value>
class HuffmanEncoder
{
    struct Code
    {
	unsigned code, bits;
	inline Code() : code(0), bits(0) { }
	inline Code(unsigned c, unsigned b) : code(c), bits(b) { }
    };
    typedef std::map<Value, Code> ValueCodeMap;
    typedef ValueCodeMap::const_iterator const_iterator;

    ValueCodeMap m_map;	// buy one, get one free!
    
public:
    ~HuffmanEncoder() { Reset(); }
    void Reset() { m_map = ValueCodeMap(); }
    void Add(const Value &value, unsigned code, unsigned bits)
    {
	m_map.insert(std::make_pair(value, Code(code, bits)));
    }
    Code Get(const Value &value) { return m_map[value]; }
};

// gcc doesn't seem to like operator< on this as a member of a template,
// so I'll gratuitously use void * here.
struct HuffmanWeightedNode
{
    unsigned weight;
    void *node;
    inline HuffmanWeightedNode(unsigned w, void *n) : weight(w), node(n) { }
};

template <class Value>
class HuffmanGenerator
{
public:
    struct Node
    {
	Value value;
	Node *left, *right;
	inline Node(const Value &v, Node *l, Node *r)
	    : value(v), left(l), right(r) { }
	inline bool IsLeaf() const { return left == NULL; }
    };

private:
    typedef std::map<Value, unsigned> ValueWeightMap;
    typedef std::multiset<HuffmanWeightedNode> Ordered;

    // Map each value to its weight (frequency count)
    ValueWeightMap m_weights;
    Node *m_root;

    void GenerateTree();
    static void Traverse(const Node *node, unsigned code, unsigned bits,
			 HuffmanEncoder<Value> *encoder);
    static void Chop(Node *node);

public:
    inline HuffmanGenerator() : m_weights(), m_root(NULL) { }
    inline ~HuffmanGenerator() { Reset(); }    
    void Reset();
    void Count(const Value &value) { ++m_weights[value]; }
    void Encode(HuffmanEncoder<Value> *encoder);
    inline const Node *GetRootNode() const { ASSERT(m_root); return m_root; }
};

// I can't get templating to instance the right function here, so I gave up.
bool operator<(const HuffmanWeightedNode &a, const HuffmanWeightedNode &b)
{
    return a.weight < b.weight;
}

template <class Value>
void HuffmanGenerator<Value>::Reset()
{
    m_weights = ValueWeightMap();
    if(m_root)
    {
	Chop(m_root);
	m_root = NULL;
    }
}

template <class Value>
void HuffmanGenerator<Value>::Chop(Node *node)
{
    if(!node->IsLeaf())
    {
	Chop(node->left);
	Chop(node->right);
    }
    delete node;
}

template <class Value>
void HuffmanGenerator<Value>::GenerateTree()
{
    // This safely handles the degenerate case of 1 node, but not 0.
    ASSERT(m_root == NULL);
    Ordered ordered;
    // Transfer nodes to ordered set
    for(ValueWeightMap::iterator it = m_weights.begin();
	it != m_weights.end(); ++it)
    {
	ordered.insert(HuffmanWeightedNode(it->second,
					   NEW Node(it->first, NULL, NULL)));
    }
    
    for(;;)
    {
	// Get two lowest weighted nodes
	Ordered::iterator left_it = ordered.begin();
	ASSERT(left_it != ordered.end());
	Ordered::iterator right_it = left_it;
	++right_it;
	if(right_it == ordered.end())
	    break;	// Only one node? Done!
	// Create a new node parenting them with the sum of their weights
	Node *parent = new Node(0,
				(Node *) left_it->node,
				(Node *) right_it->node);
	HuffmanWeightedNode weight(left_it->weight + right_it->weight, parent);
	ordered.erase(left_it);
	ordered.erase(right_it);
	ordered.insert(weight);
    }
    ASSERT(!ordered.empty());
    m_root = (Node *) ordered.begin()->node;	// Only node left is the root
}

template <class Value>
void HuffmanGenerator<Value>::Traverse(const Node *node,
				       unsigned code, unsigned bits,
				       HuffmanEncoder<Value> *encoder)
{
    ASSERT(bits <= 32);
    if(!node->IsLeaf())
    {
	Traverse(node->left, code, bits + 1, encoder);
	Traverse(node->right, code | (1 << bits), bits + 1, encoder);
    }
    else
	encoder->Add(node->value, code, bits); // Leaf, generate code
}

template <class Value>
void HuffmanGenerator<Value>::Encode(HuffmanEncoder<Value> *encoder)
{
    GenerateTree();
    Traverse(GetRootNode(), 0, 0, encoder);
}

#endif
