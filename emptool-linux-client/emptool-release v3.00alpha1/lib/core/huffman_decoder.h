/* huffman_decoder.h
 *
 * (C) 2003 empeg ltd, http://www.empeg.com
 *
 * This software is licensed under the GNU General Public Licence (see file
 * COPYING), unless you possess an alternative written licence from empeg ltd.
 *
 * (:Empeg Source Release 1.2 13-Mar-2003 18:15 rob:)
 */

#ifndef HUFFMAN_DECODER_H
#define HUFFMAN_DECODER_H	1

/* Incredibly inefficient generic Huffman Decoder.
 *
 * Usage:
 *
 *     HuffmanDecoder<unsigned> decoder;
 *
 *     while(not_eof)
 *     {
 *         unsigned code = GetNextCode();
 *         unsigned bits = GetNextNumberOfBits();
 *         unsigned value = GetNextResultingValue();
 *         decoder.AddCode(code, bits, value);
 *     }
 *
 *     while(not_eof)
 *     {
 *         unsigned bit = GetNextBit();
 *         unsigned value;
 *         if(decoder.DecodeBit(bit, &value))
 *             printf("Got value %u\n", value);
 *     }
 *
 *     decoder.Reset();
 *     ...
 */

template <class Value>
class HuffmanDecoder
{
    struct Node
    {
	Value value;
	Node *left, *right;
	Node(unsigned v, Node *l, Node *r) : value(v), left(l), right(r) { }
    };

    Node *m_root, *m_current;

    void RecursiveDelete(Node *node);
    
public:
    HuffmanDecoder() : m_root(NULL), m_current(NULL) { Reset(); }
    ~HuffmanDecoder() { Reset(); }
    void Reset();
    void AddCode(unsigned code, unsigned bits, const Value &value);
    bool DecodeBit(unsigned bit, Value *result);
};

template <class Value>
void HuffmanDecoder<Value>::Reset()
{
    if(m_root)
	RecursiveDelete(m_root);
    m_root = NEW Node(Value(), NULL, NULL);
    m_current = m_root;
}

template <class Value>
void HuffmanDecoder<Value>::RecursiveDelete(Node *node)
{
    if(node->left)
	RecursiveDelete(node->left);
    if(node->right)
	RecursiveDelete(node->right);
    delete node;
}

template <class Value>
void HuffmanDecoder<Value>::AddCode(unsigned code, unsigned bits,
				    const Value &value)
{
    // Descend the tree, creating branches as necessary
    Node *node = m_root;
    ASSERT(bits > 0);
    do
    {
	ASSERT(node->value == Value());
	Node **choice = (code & 1) ? &node->right : &node->left;
	if(!*choice)
	    *choice = NEW Node(Value(), NULL, NULL);
	node = *choice;
	code >>= 1;
	--bits;
    } while(bits > 0);
    // At the leaf node, set its value
    node->value = value;
}

template <class Value>
bool HuffmanDecoder<Value>::DecodeBit(unsigned bit, Value *result)
{
    // Descend left if 0, right if 1
    Node **choice = bit ? &m_current->right : &m_current->left;
    ASSERT_VALID(*choice);
    m_current = *choice;
    if(!m_current->left && !m_current->right)
    {
	*result = m_current->value;
	m_current = m_root;
	return true;	// We have a leaf
    }
    else
	return false;
}

#endif
