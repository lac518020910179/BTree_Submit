# include "utility.hpp"
# include <fstream>
# include <functional>
# include <cstddef>
# include "exception.hpp"
# include <map>
namespace sjtu {
    int ID = 0;
template <class Key, class Value, class Compare = std::less<Key> >
class BTree {
public:
    typedef pair <Key, Value> value_type;

	typedef ssize_t node_t;
    typedef ssize_t offset_t;

    class iterator;
    class const_iterator;


private:
    static const int M=500;
    static const int L=250;
    static const int info_offset=0;


	struct fileName{
        char *str;
        fileName() {str = new char[10];}
        ~fileName() {delete str;}
        void setName(int id)
        {
            if (id<0 || id>9) throw "no more B plus Tree!";
            str[0] = 'd';
            str[1] = 'a';
            str[2] = 't';
            str[3] = static_cast<char> (id + '0');
            str[4] = '.'; 
            str[5] = 'd'; 
            str[6] = 'a';
            str[7] = 't'; 
            str[8] = '\0';
        }
        void setName(char *_str)
        {
            int i;
            for (i=0; _str[i]!='\0'; i++) str[i] = _str[i];
            str[i] = '\0';
        }
    };


    struct bptInfo{
        node_t head;     //head of leaf
        node_t tail;     //tail of leaf
        node_t root;     //root of BTree
        size_t size;     //size of BTree
        offset_t eof;    //end of file

        bptInfo()
        {
            head = 0;
            tail = 0;
            root = 0;
            size = 0;
            eof = 0;
        }
    };


	struct leafNode{
        offset_t offset;
        node_t par;               //parent
        node_t pre;
        node_t nxt;
        int cnt;                 //number of pairs in leaf
        value_type data[L+1];
        leafNode()
        {
            offset = 0;
            par = 0;
            cnt = 0;
        }
    };


    struct internalNode
    {
        offset_t offset;
        node_t par;
        node_t ch[M + 1];
        Key key[M + 1];
        int cnt;
        bool type;
        internalNode()
        {
            offset = 0;
            par = 0;
            for (int i = 0; i <= M; ++i) ch[i] = 0;
            cnt = 0;
            type = 0;
        }
    };


	FILE *fp;
    fileName fp_name;
    bptInfo info;
    bool fp_open;
    bool file_already_exists;


    inline void openFile()
    {
        file_already_exists = 1;
        if (fp_open == 0) {
            fp = fopen(fp_name.str, "rb+");
            if (fp == nullptr) {
                file_already_exists = 0;
                fp = fopen(fp_name.str, "w");
                fclose(fp);
                fp = fopen(fp_name.str, "rb+");
            } else readFile(&info, info_offset, 1, sizeof(bptInfo));
            fp_open = 1;
        }
    }


	inline void closeFile()
    {
        if (fp_open==1) fclose(fp);
        fp_open = 0;
    }


    inline void readFile(void *place, offset_t offset, size_t num, size_t size) const
    {
        if (fseek(fp, offset, SEEK_SET)) throw "open file failed!";
        fread(place, size, num, fp);
    }


    inline void writeFile(void *place, offset_t offset, size_t num, size_t size) const
    {
        if (fseek(fp, offset, SEEK_SET)) throw "open file failed!";
        fwrite(place, size, num, fp);
    }


    fileName fp_from_name;
    FILE *fp_from;


    inline void copy_readFile(void *place, offset_t offset, size_t num, size_t size) const
    {
        if (fseek(fp_from, offset, SEEK_SET)) throw "open file failed!";
        fread(place, size, num, fp_from);
    }


	offset_t leaf_offset_temp;
    inline void copy_leaf(offset_t offset, offset_t from_offset, offset_t par_offset)
    {
        leafNode leaf, leaf_from, pre_leaf;
        copy_readFile(&leaf_from, from_offset, 1, sizeof(leafNode));

        leaf.offset = offset;
        leaf.par = par_offset;
        leaf.cnt = leaf_from.cnt;
        leaf.pre = leaf_offset_temp;
        leaf.nxt = 0;
        if (leaf_offset_temp!=0)
        {
            readFile(&pre_leaf, leaf_offset_temp, 1, sizeof(leafNode));
            pre_leaf.nxt = offset;
            writeFile(&pre_leaf, leaf_offset_temp, 1, sizeof(leafNode));
            info.tail = offset;
        } else info.head = offset;

        for (int i=0; i<leaf.cnt; i++)
        {
            leaf.data[i].first = leaf_from.data[i].first;
            leaf.data[i].second = leaf_from.data[i].second;
        }

        writeFile(&leaf, offset, 1, sizeof(leafNode));

        info.eof += sizeof(leafNode);
        leaf_offset_temp = offset;
    }


    inline void copy_node(offset_t offset, offset_t from_offset, offset_t par_offset)
    {
        internalNode node, node_from;
        copy_readFile(&node_from, from_offset, 1, sizeof(internalNode));
        writeFile(&node, offset, 1, sizeof(internalNode));

        info.eof += sizeof(internalNode);
        node.offset = offset;
        node.par = par_offset;
        node.cnt = node_from.cnt;
        node.type = node_from.type;

        for (int i=0; i<node.cnt; i++)
        {
            node.key[i] = node_from.key[i];
            if (node.type==1) copy_leaf(info.eof, node_from.ch[i], offset);
            else copy_node(info.eof, node_from.ch[i], offset);
        }

        writeFile(&node, offset, 1, sizeof(internalNode));
    }


	inline void copyFile(char *to, char *from)
    {
        fp_from_name.setName(from);
        fp_from = fopen(fp_from_name.str, "rb+");
        if (fp_from==nullptr) throw "no such file";
        bptInfo infoo;
        copy_readFile(&infoo, info_offset, 1, sizeof(bptInfo));

        leaf_offset_temp = 0;
        info.size = infoo.size;
        info.root = info.eof = sizeof(bptInfo);

        writeFile(info, info_offset, 1, sizeof(bptInfo));
        copy_node(info.root, infoo.root, 0);
        writeFile(info, info_offset, 1, sizeof(bptInfo));
        fclose(fp_from);
    }


    inline void build_tree()
    {
        info.size = 0;
        info.eof = sizeof(bptInfo);

        internalNode root;
        info.root = root.offset = info.eof;
        info.eof += sizeof(internalNode);

        leafNode leaf;
        info.head = info.tail = leaf.offset = info.eof;
        info.eof += sizeof(leafNode);

        root.par=0; root.cnt=1; root.type=1;
        root.ch[0] = leaf.offset;

        leaf.par = root.offset;
        leaf.pre = leaf.nxt = 0;
        leaf.cnt = 0;

        writeFile(&info, info_offset, 1, sizeof(bptInfo));
        writeFile(&root, root.offset, 1, sizeof(internalNode));
        writeFile(&leaf, leaf.offset, 1, sizeof(leafNode));
    }


    node_t locate_leaf(const Key &key, offset_t offset) const
    {
        internalNode p;
        readFile(&p, offset, 1, sizeof(internalNode));

        if (p.type==1)
        {
            int pos;
            for (pos=0; pos<p.cnt; pos++)
                if (key<p.key[pos]) break;
            if (pos==0) return 0;
            return p.ch[pos-1];
        }
        else
        {
            int pos;
            for (pos=0; pos<p.cnt; pos++)
                if (key<p.key[pos]) break;
            if (pos==0) return 0;
            return locate_leaf(key, p.ch[pos-1]);
        }
    }

			/**

			 * function: insert an element (key, value) to the given leaf.

			 * return Fail and operate nothing if there are elements with same key.

			 * return Success if inserted.

			 * if leaf count is bigger than L then call split_leaf().

			 */

    pair <iterator, OperationResult> insert_leaf(leafNode &leaf, const Key &key, const Value &value)
    {
        iterator ret;
        int pos = 0;

        for (; pos < leaf.cnt; ++pos) {
            if (key == leaf.data[pos].first) return pair <iterator, OperationResult> (iterator(nullptr), Fail);			// there are elements with the same key
            if (key < leaf.data[pos].first) break;
        }

        for (int i = leaf.cnt - 1; i >= pos; --i)
        {
            leaf.data[i+1].first = leaf.data[i].first;
            leaf.data[i+1].second = leaf.data[i].second;
        }
        leaf.data[pos].first = key;
        leaf.data[pos].second = value;

        ++leaf.cnt;
        ++info.size;
        ret.from = this;
        ret.offset = leaf.offset;
        ret.place = pos;

        writeFile(&info, info_offset, 1, sizeof(bptInfo));
        if(leaf.cnt <= L) writeFile(&leaf, leaf.offset, 1, sizeof(leafNode));
        else split_leaf(leaf, ret, key);
        return pair <iterator, OperationResult> (ret, Success);
    }


	void insert_node(internalNode &node, const Key &key, node_t ch)
    {
        int pos;

        for (pos=0; pos<node.cnt&&key>=node.key[pos]; pos++);
        for (int i=node.cnt-1; i>=pos; i--)
        {
            node.key[i+1] = node.key[i];
            node.ch[i+1] = node.ch[i];
        }
        node.key[pos] = key;
        node.ch[pos] = ch;
        node.cnt++;

        if (node.cnt<=M) writeFile(&node, node.offset, 1, sizeof(internalNode));
        else split_node(node);
    }


	void split_leaf(leafNode &leaf, iterator &it, const Key &key)
    {
        leafNode newleaf;
        newleaf.cnt = leaf.cnt-(leaf.cnt/2);
        leaf.cnt = leaf.cnt/2;
        newleaf.par = leaf.par;
        newleaf.offset = info.eof;
        info.eof += sizeof(leafNode);

        for (int i=0; i<newleaf.cnt; i++)
        {
            newleaf.data[i].first = leaf.data[i+leaf.cnt].first;
            newleaf.data[i].second = leaf.data[i+leaf.cnt].second;
            if (newleaf.data[i].first==key)
            {
                it.offset = newleaf.offset;
                it.place = i;
            }
        }

        newleaf.nxt = leaf.nxt;
        newleaf.pre = leaf.offset;
        leaf.nxt = newleaf.offset;
        leafNode nxtleaf;
        if (newleaf.nxt==0) info.tail = newleaf.offset;
        else
        {
            readFile(&nxtleaf, newleaf.nxt, 1, sizeof(leafNode));
            nxtleaf.pre = newleaf.offset;
            writeFile(&nxtleaf, nxtleaf.offset, 1, sizeof(leafNode));
        }

        writeFile(&info, info_offset, 1, sizeof(bptInfo));
        writeFile(&leaf, leaf.offset, 1, sizeof(leafNode));
        writeFile(&newleaf, newleaf.offset, 1, sizeof(leafNode));

        internalNode par;
        readFile(&par, leaf.par, 1, sizeof(internalNode));
        insert_node(par, newleaf.data[0].first, newleaf.offset);
    }


	void split_node(internalNode &node)
    {
        internalNode newnode;
        newnode.cnt = node.cnt-node.cnt/2;
        node.cnt = node.cnt/2;
        newnode.par = node.par;
        newnode.type = node.type;
        newnode.offset = info.eof;
        info.eof += sizeof(internalNode);

        for (int i=0; i<newnode.cnt; i++)
        {
            newnode.key[i] = node.key[i+node.cnt];
            newnode.ch[i] = node.ch[i+node.cnt];
        }

        leafNode leaf;
        internalNode internal;
        for (int i=0; i<newnode.cnt; i++)
        {
            if (node.type==1)
            {
                readFile(&leaf, newnode.ch[i], 1, sizeof(leafNode));
                leaf.par = newnode.offset;
                writeFile(&leaf, newnode.ch[i], 1, sizeof(leafNode));
            }
            else
            {
                readFile(&internal, newnode.ch[i], 1, sizeof(internalNode));
                internal.par = newnode.offset;
                writeFile(&internal, newnode.ch[i], 1, sizeof(internalNode));
            }
        }

        if (node.offset==info.root)
        {
            internalNode newroot;
            newroot.par = 0;
            newroot.type = 0;
            newroot.offset = info.eof;
            info.eof += sizeof(internalNode);
            info.root = newroot.offset;
            newroot.cnt = 2;
            newroot.ch[0] = node.offset;
            newroot.ch[1] = newnode.offset;
            newroot.key[0] = node.key[0];
            newroot.key[1] = newnode.key[0];
            node.par = newroot.offset;
            newnode.par = newroot.offset;

            writeFile(&info, info_offset, 1, sizeof(bptInfo));
            writeFile(&node, node.offset, 1, sizeof(internalNode));
            writeFile(&newnode, newnode.offset, 1, sizeof(internalNode));
            writeFile(&newroot, newroot.offset, 1, sizeof(internalNode));
        }
        else
        {
            writeFile(&info, info_offset, 1, sizeof(bptInfo));
            writeFile(&node, node.offset, 1, sizeof(internalNode));
            writeFile(&newnode, newnode.offset, 1, sizeof(internalNode));

            internalNode par;
            readFile(&par, node.par, 1, sizeof(internalNode));
            insert_node(par, newnode.key[0], newnode.offset);
        }
    }

public:
  class iterator {
      friend class BTree;
   private:
       offset_t offset;
       int place;
       BTree *from;

   public:
    iterator() {
        offset = 0;
        place = 0;
        from = nullptr;
    }

    iterator(BTree *_from, offset_t _offset=0, int _place=0)
    {
        from = _from;
        offset = _offset;
        place = _place;
    }

    iterator(const iterator& other) {
        offset = other.offset;
        place = other.place;
        from = other.from;
    }


    Value getValue()
    {
        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        return p.data[place].second;
    }


    OperationResult modify(const Value &value)
    {
        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        p.data[place].second = value;
        from->writeFile(&p, offset, 1, sizeof(leafNode));
        return Success;
    }

    // Return a new iterator which points to the n-next elements
    iterator operator++(int) {
        iterator ret = *this;

        if (*this==from->end())
        {
            from = nullptr; offset = 0; place = 0;
            return ret;
        }

        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==p.cnt-1)
        {
            if (p.nxt==0) place++;
            else
            {
                offset = p.nxt;
                place = 0;
            }
        } else place++;
        return ret;
    }


    iterator& operator++() {
        if (*this==from->end())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return *this;
        }

        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==p.cnt-1)
        {
            if (p.nxt==0) place++;
            else
            {
                offset = p.nxt;
                place = 0;
            }
        } else place++;
        return *this;
    }


    iterator operator--(int) {
        iterator ret = *this;

        if (*this==from->begin())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return ret;
        }

        leafNode p, q;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->readFile(&q, p.pre, 1, sizeof(leafNode));
            place = q.cnt-1;
        } else place--;
        return ret;
    }


    iterator& operator--() {
        if (*this==from->begin())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return *this;
        }

        leafNode p, q;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->readFile(&q, p.pre, 1, sizeof(leafNode));
            place = q.cnt-1;
        } else place--;
        return *this;
    }


    // Overloaded of operator '==' and '!='
    // Check whether the iterators are same
    bool operator==(const iterator& rhs) const {
        return (from==rhs.from && offset==rhs.offset && place==rhs.place);
    }
    bool operator==(const const_iterator& rhs) const {
        return (from==rhs.from && offset==rhs.offset && place==rhs.place);
    }
    bool operator!=(const iterator& rhs) const {
        return (from!=rhs.from || offset!=rhs.offset || place!=rhs.place);
    }
    bool operator!=(const const_iterator& rhs) const {
        return (from!=rhs.from || offset!=rhs.offset || place!=rhs.place);
    }
  };



 class const_iterator {
    friend class BTree;
  private:
    offset_t offset;        // offset of the leaf node
    int place;				// place of the element in the leaf node
    const BTree *from;
  public:
    const_iterator() {
        from = nullptr;
        place = 0, offset = 0;
    }
    const_iterator(const BTree *_from, offset_t _offset = 0, int _place = 0) {
        from = _from;
        offset = _offset; place = _place;
    }
    const_iterator(const iterator& other) {
        from = other.from;
        offset = other.offset;
        place = other.place;
    }
    const_iterator(const const_iterator& other) {
        from = other.from;
        offset = other.offset;
        place = other.place;
    }

    // to get the value type pointed by iterator.
	Value getValue()
    {
        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        return p.data[place].second;
    }

    // Return a new iterator which points to the n-next elements
	const_iterator operator++(int) {
        const_iterator ret = *this;

        if (*this==from->cend())
        {
            from = nullptr; offset = 0; place = 0;
            return ret;
        }

        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==p.cnt-1)
        {
            if (p.nxt==0) place++;
            else
            {
                offset = p.nxt;
                place = 0;
            }
        } else place++;
        return ret;
    }


    const_iterator& operator++() {
        if (*this==from->cend())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return *this;
        }

        leafNode p;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==p.cnt-1)
        {
            if (p.nxt==0) place++;
            else
            {
                offset = p.nxt;
                place = 0;
            }
        } else place++;
        return *this;
    }


    const_iterator operator--(int) {
        const_iterator ret = *this;

        if (*this==from->cbegin())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return ret;
        }

        leafNode p, q;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->readFile(&q, p.pre, 1, sizeof(leafNode));
            place = q.cnt-1;
        } else place--;
        return ret;
    }


    const_iterator& operator--() {
        if (*this==from->cbegin())
        {
            from = nullptr;
            offset = 0;
            place = 0;
            return *this;
        }

        leafNode p, q;
        from->readFile(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->readFile(&q, p.pre, 1, sizeof(leafNode));
            place = q.cnt-1;
        } else place--;
        return *this;
    }


    bool operator==(const iterator& rhs) const {
        return (from==rhs.from && offset==rhs.offset && place==rhs.place);
    }
    bool operator==(const const_iterator& rhs) const {
        return (from==rhs.from && offset==rhs.offset && place==rhs.place);
    }
    bool operator!=(const iterator& rhs) const {
        return (from!=rhs.from || offset!=rhs.offset || place!=rhs.place);
    }
    bool operator!=(const const_iterator& rhs) const {
        return (from!=rhs.from || offset!=rhs.offset || place!=rhs.place);
    }
  };



  // Default Constructor and Copy Constructor
  BTree() {
      fp_name.setName(ID);
      fp = nullptr;
      openFile();
      if (file_already_exists==0) build_tree();
  }


  BTree(const BTree& other) {
      fp_name.setName(ID);
      openFile();
      copyFile(fp_name.str, other.fp_name.str);
  }


  BTree& operator=(const BTree& other) {
      fp_name.setName(ID);
      openFile();
      copyFile(fp_name.str, other.fp_name.str);
  }


  ~BTree() {
      closeFile();
  }


  // Insert: Insert certain Key-Value into the database
  // Return a pair, the first of the pair is the iterator point to the new
  // element, the second of the pair is Success if it is successfully inserted
  pair<iterator, OperationResult> insert(const Key& key, const Value& value)
  {
      offset_t leaf_offset = locate_leaf(key, info.root);
      leafNode leaf;

      if (info.size==0 || leaf_offset==0)
      {
          readFile(&leaf, info.head, 1, sizeof(leafNode));
          pair<iterator, OperationResult> ret = insert_leaf(leaf, key, value);
          if (ret.second==Fail) return ret;

          offset_t offset = leaf.par;
          internalNode node;
          while(offset!=0)
          {
              readFile(&node, offset, 1, sizeof(internalNode));
              node.key[0] = key;
              writeFile(&node, offset, 1, sizeof(internalNode));
              offset = node.par;
          }
          return ret;
      }

      readFile(&leaf, leaf_offset, 1, sizeof(leafNode));
      pair<iterator,OperationResult>ret = insert_leaf(leaf, key, value);
      return ret;
  }


  // Erase: Erase the Key-Value
  // Return Success if it is successfully erased
  // Return Fail if the key doesn't exist in the database
  OperationResult erase(const Key& key) {

      return Fail;  // If you can't finish erase part, just remaining here.
  }


  Value at(const Key& key)
  {
      iterator it = find(key);
      if(it==end()) throw "not found";

      leafNode leaf;
      readFile(&leaf, it.offset, 1, sizeof(leafNode));
      return leaf.data[it.place].second;
  }


  const Value& at(const Key& key) const
  {
      iterator it = find(key);
      if (it==end()) throw "not found";

      leafNode leaf;
      readFile(&leaf, it.offset, 1, sizeof(leafNode));
      return leaf.data[it.place].second;
  }


  // Return a iterator to the beginning
  iterator begin() {return iterator(this, info.head, 0);}

  const_iterator cbegin() const {return const_iterator(this, info.head, 0);}

  // Return a iterator to the end(the next element after the last)
  iterator end()
  {
      leafNode tail;
      readFile(&tail, info.tail, 1, sizeof(leafNode));
      return iterator(this, info.tail, tail.cnt);
  }

  const_iterator cend() const
  {
      leafNode tail;
      readFile(&tail, info.tail, 1, sizeof(leafNode));
      return const_iterator(this, info.tail, tail.cnt);
  }

  // Check whether this BTree is empty
  bool empty() const {return info.size==0;}

  // Return the number of <K,V> pairs
  size_t size() const {return info.size;}

  // Clear the BTree
  void clear()
  {
      fp = fopen(fp_name.str, "w");
      fclose(fp);
      openFile();
      build_tree();
  }


  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key& key) const
  {
      return static_cast<size_t>(find(key)!=cend());
  }


  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is
   * returned.
   */
  iterator find(const Key& key)
  {
      offset_t leaf_offset = locate_leaf(key, info.root);
      if (leaf_offset==0) return end();

      leafNode leaf;
      readFile(&leaf, leaf_offset, 1, sizeof(leafNode));
      for (int i=0; i<leaf.cnt; i++)
        if (leaf.data[i].first==key) return iterator(this, leaf_offset, i);
      return end();
  }


  const_iterator find(const Key& key) const
  {
      offset_t leaf_offset;
      if (leaf_offset==0) return cend();

      leafNode leaf;
      readFile(&leaf, leaf_offset, 1, sizeof(leafNode));
      for (int i=0; i<leaf.cnt; i++)
        if (leaf.data[i].first==key) return const_iterator(this, leaf_offset, i);
      return cend();
  }
};
}  // namespace sjtu
