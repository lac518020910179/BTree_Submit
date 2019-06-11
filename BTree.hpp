# include "utility.hpp"
# include <fstream>
# include <functional>
# include <cstddef>
# include "exception.hpp"

namespace sjtu {
    const int maxn=4096;
    int ID = 0;
template <class Key, class Value, class Compare = std::less<Key> >
class BTree {
public:
    typedef pair <Key, Value> value_type;
    class iterator;
    class const_iterator;
private:
    static const int M = (maxn/(sizeof(ssize_t)+sizeof(Key)))<5? 4:(maxn/(sizeof(ssize_t)+sizeof(Key))-1);
    static const int L = (maxn/(sizeof(value_type)))<5? 4:(maxn/(sizeof(value_type))-1);
    static const int MMIN = (M+1)/2;
    static const int LMIN = (L+1)/2;
    static const int info_offset = 0;


	struct FileName{
        char *str;
        FileName() {str = new char[10];}
        ~FileName() {delete str;}
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


    struct bpt_Info{
        ssize_t head;     //head of leaf
        ssize_t tail;     //tail of leaf
        ssize_t root;     //root of BTree
        size_t size;     //size of BTree
        ssize_t eof;    //end of file

        bpt_Info()
        {
            head = 0;
            tail = 0;
            root = 0;
            size = 0;
            eof = 0;
        }
    };


	struct leafNode{
        ssize_t offset;
        ssize_t parent;
        ssize_t pre;
        ssize_t next;
        int num;                 //number of pairs in leaf
        value_type data[L+1];
        leafNode()
        {
            offset = 0;
            parent = 0;
            num = 0;
        }
    };


    struct internalNode
    {
        ssize_t offset;
        ssize_t parent;
        ssize_t chlid[M + 1];
        Key key[M + 1];
        int num;
        bool type;//child is leaf or not
        internalNode()
        {
            offset=0;
            parent=0;
            for (int i=0; i<=M;++i)child[i]=0;
            num=0;
            type=0;
        }
    };


	FILE *fp;
    FileName fp_name;
    bpt_Info info;
    bool fp_open;
    bool exist;


    void open()
    {
        exist=1;
        if (fp_open==0) {
            fp=fopen(fp_name.str,"rb+");
            if (fp==nullptr)
            {
                exist=0;
                fp = fopen(fp_name.str,"w");
                fclose(fp);
                fp=fopen(fp_name.str,"rb+");
            }
            else read(&info,info_offset,1,sizeof(bpt_Info));
            fp_open=1;
        }
    }


	void close()
    {
        if(fp_open==1)fclose(fp);
        fp_open=0;
    }


    void read(void *place, ssize_t offset, size_t num, size_t size) const
    {
        if (fseek(fp,offset,SEEK_SET)) throw "open file failed!";
        fread(place,size,num,fp);
    }


    void write(void *place, ssize_t offset, size_t num, size_t size) const
    {
        if (fseek(fp,offset, SEEK_SET)) throw "open file failed!";
        fwrite(place,size,num,fp);
    }


    FileName fp_from_name;
    FILE *fp_from;


    void copy_read(void *place,ssize_t offset, size_t num, size_t size) const
    {
        if (fseek(fp_from, offset, SEEK_SET)) throw "open file failed!";
        fread(place,size,num, fp_from);
    }


	ssize_t leaf_offset_temp;
    void copy_leaf(ssize_t offset, ssize_t from_offset, ssize_t parent_offset)
    {
        leafNode leaf, leaf_from, pre_leaf;
        copy_read(&leaf_from, from_offset, 1, sizeof(leafNode));

        leaf.offset=offset;
        leaf.parent=parent_offset;
        leaf.num=leaf_from.num;
        leaf.pre=leaf_offset_temp;
        leaf.next=0;
        if (leaf_offset_temp!=0)
        {
            read(&pre_leaf, leaf_offset_temp,1,sizeof(leafNode));
            pre_leaf.next=offset;
            write(&pre_leaf,leaf_offset_temp,1,sizeof(leafNode));
            info.tail=offset;
        } else info.head=offset;

        for (int i=0;i<leaf.num; i++)
        {
            leaf.data[i].first=leaf_from.data[i].first;
            leaf.data[i].second=leaf_from.data[i].second;
        }

        writeFile(&leaf,offset,1,sizeof(leafNode));

        info.eof+=sizeof(leafNode);
        leaf_offset_temp=offset;
    }


    void copy_node(ssize_t offset, ssize_t from_offset, ssize_t parent_offset)
    {
        internalNode node, node_from;
        copy_readFile(&node_from, from_offset, 1, sizeof(internalNode));
        write(&node, offset,1,sizeof(internalNode));

        info.eof += sizeof(internalNode);
        node.offset = offset;
        node.parent = parent_offset;
        node.num = node_from.num;
        node.type = node_from.type;

        for (int i=0; i<node.cnt; i++)
        {
            node.key[i] = node_from.key[i];
            if (node.type==1) copy_leaf(info.eof, node_from.child[i],offset);
            else copy_node(info.eof, node_from.child[i],offset);
        }

        write(&node, offset, 1, sizeof(internalNode));
    }


	void copy(char *to, char *from)
    {
        fp_from_name.setName(from);
        fp_from=fopen(fp_from_name.str, "rb+");
        if (fp_from==nullptr) throw "no such file";
        bpt_Info infoo;
        copy_read(&infoo, info_offset, 1, sizeof(bpt_Info));

        leaf_offset_temp = 0;
        info.size = infoo.size;
        info.root = info.eof = sizeof(bpt_Info);

        write(info,info_offset,1,sizeof(bpt_Info));
        copy_node(info.root, infoo.root, 0);
        write(info,info_offset, 1, sizeof(bpt_Info));
        fclose(fp_from);
    }


    void initialbuild()
    {
        info.size = 0;
        info.eof = sizeof(bpt_Info);

        internalNode root;
        info.root=root.offset=info.eof;
        info.eof+=sizeof(internalNode);

        leafNode leaf;
        info.head=info.tail=leaf.offset=info.eof;
        info.eof+=sizeof(leafNode);

        root.parent=0;
        root.num=1;
        root.type=1;
        root.child[0]=leaf.offset;

        leaf.parent=root.offset;
        leaf.pre=leaf.next = 0;
        leaf.num=0;

        write(&info,info_offset,1,sizeof(bpt_Info));
        write(&root,root.offset,1,sizeof(internalNode));
        write(&leaf,leaf.offset,1,sizeof(leafNode));
    }


    ssize_t locate_leaf(const Key &key, ssize_t offset) const
    {
        internalNode p;
        read(&p,offset,1,sizeof(internalNode));

        if (p.type==1)
        {
            int pos;
            for (pos=0;pos<p.num;pos++)
                if (key<p.key[pos]) break;
            if (pos==0) return 0;
            return p.child[pos-1];
        }
        else
        {
            int pos;
            for (pos=0;pos<p.num;pos++)
                if (key<p.key[pos])break;
            if (pos==0) return 0;
            return locate_leaf(key, p.child[pos-1]);
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
        int pos=0;

        for (; pos<leaf.num&&key>=leaf.data[pos].first;++pos) {
            if (key==leaf.data[pos].first) return pair <iterator, OperationResult> (iterator(nullptr), Fail);
        }

        for (int i=leaf.num-1;i>=pos;--i)
        {
            leaf.data[i+1].first = leaf.data[i].first;
            leaf.data[i+1].second = leaf.data[i].second;
        }
        leaf.data[pos].first=key;
        leaf.data[pos].second=value;

        ++leaf.num;
        ++info.size;
        ret.from=this;
        ret.offset=leaf.offset;
        ret.place=pos;

        write(&info, info_offset, 1, sizeof(bpt_Info));
        if(leaf.num<=L) write(&leaf,leaf.offset,1,sizeof(leafNode));
        else split_leaf(leaf,ret,key);
        return pair <iterator,OperationResult> (ret, Success);
    }


	void insert_node(internalNode &node, const Key &key, node_t ch)
    {
        int pos;

        for (pos=0; pos<node.num&&k>=node.key[pos]; pos++);
        for (int i=node.num-1;i>=pos; i--)
        {
            node.key[i+1] = node.key[i];
            node.child[i+1] = node.child[i];
        }
        node.key[pos] = key;
        node.child[pos] = ch;
        node.num++;

        if (node.num<=M) write(&node, node.offset, 1, sizeof(internalNode));
        else split_node(node);
    }


	void split_leaf(leafNode &leaf, iterator &it, const Key &key)
    {
        leafNode newleaf;
        newleaf.num = leaf.num-(leaf.num/2);
        leaf.num/=2;
        newleaf.parent = leaf.parent;
        newleaf.offset = info.eof;
        info.eof += sizeof(leafNode);

        for (int i=0; i<newleaf.num; i++)
        {
            newleaf.data[i].first = leaf.data[i+leaf.num].first;
            newleaf.data[i].second = leaf.data[i+leaf.num].second;
            if (newleaf.data[i].first==key)
            {
                it.offset = newleaf.offset;
                it.place = i;
            }
        }

        newleaf.next = leaf.next;
        newleaf.pre = leaf.offset;
        leaf.next = newleaf.offset;
        leafNode nextleaf;
        if (newleaf.next==0) info.tail = newleaf.offset;
        else
        {
            read(&nextleaf, newleaf.next, 1, sizeof(leafNode));
            nextleaf.pre = newleaf.offset;
            write(&nextleaf, nextleaf.offset, 1, sizeof(leafNode));
        }

        write(&info,info_offset,1,sizeof(bpt_Info));
        write(&leaf,leaf.offset,1,sizeof(leafNode));
        write(&newleaf,newleaf.offset,1,sizeof(leafNode));

        internalNode parent;
        readFile(&parent, leaf.parent, 1, sizeof(internalNode));
        insert_node(parent, newleaf.data[0].first, newleaf.offset);
    }


	void split_node(internalNode &node)
    {
        internalNode newnode;
        newnode.num= node.num-node.num/2;
        node.num/=2;
        newnode.parent = node.parent;
        newnode.type = node.type;
        newnode.offset = info.eof;
        info.eof += sizeof(internalNode);

        for (int i=0; i<newnode.num; i++)
        {
            newnode.key[i] = node.key[i+node.num];
            newnode.child[i] = node.child[i+node.num];
        }

        leafNode leaf;
        internalNode internal;
        for (int i=0; i<newnode.num; i++)
        {
            if (node.type==1)
            {
                read(&leaf, newnode.child[i], 1, sizeof(leafNode));
                leaf.parent = newnode.offset;
                write(&leaf, newnode.child[i], 1, sizeof(leafNode));
            }
            else
            {
                read(&internal, newnode.child[i], 1, sizeof(internalNode));
                internal.par = newnode.offset;
                write(&internal, newnode.child[i], 1, sizeof(internalNode));
            }
        }

        if (node.offset==info.root)
        {
            internalNode newroot;
            newroot.parent = 0;
            newroot.type = 0;
            newroot.offset = info.eof;
            info.eof += sizeof(internalNode);
            info.root = newroot.offset;
            newroot.num= 2;
            newroot.child[0] = node.offset;
            newroot.child[1] = newnode.offset;
            newroot.key[0] = node.key[0];
            newroot.key[1] = newnode.key[0];
            node.parent = newroot.offset;
            newnode.parent = newroot.offset;

            write(&info, info_offset, 1, sizeof(bpt_Info));
            write(&node, node.offset, 1, sizeof(internalNode));
            write(&newnode, newnode.offset, 1, sizeof(internalNode));
            write(&newroot, newroot.offset, 1, sizeof(internalNode));
        }
        else
        {
            writeFile(&info, info_offset, 1, sizeof(bpt_Info));
            writeFile(&node, node.offset, 1, sizeof(internalNode));
            writeFile(&newnode, newnode.offset, 1, sizeof(internalNode));

            internalNode parent;
            read(&parent, node.parent, 1, sizeof(internalNode));
            insert_node(parent, newnode.key[0], newnode.offset);
        }
    }
public:
  class iterator {
      friend class BTree;
   private:
       ssize_t offset;
       int place;
       BTree *from;

   public:
    iterator() {
        offset = 0;
        place = 0;
        from = nullptr;
    }

    iterator(BTree *_from, ssize_t _offset=0, int _place=0)
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
        from->read(&p, offset, 1, sizeof(leafNode));
        return p.data[place].second;
    }


    OperationResult modify(const Value &value)
    {
        leafNode p;
        from->read(&p, offset, 1, sizeof(leafNode));
        p.data[place].second = value;
        from->write(&p, offset, 1, sizeof(leafNode));
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
        from->read(&p, offset,1,sizeof(leafNode));
        if (place==p.num-1)
        {
            if (p.next==0) place++;
            else
            {
                offset = p.next;
                place = 0;
            }
        }
        else place++;
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
        from->read(&p, offset, 1, sizeof(leafNode));
        if (place==p.num-1)
        {
            if (p.next==0) place++;
            else
            {
                offset = p.next;
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
        from->read(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->read(&q, p.pre, 1, sizeof(leafNode));
            place = q.num-1;
        }
        else place--;
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
        from->read(&p, offset, 1, sizeof(leafNode));
        if (place==0)
        {
            offset = p.pre;
            from->read(&q, p.pre, 1, sizeof(leafNode));
            place = q.num-1;
        }
        else place--;
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
    ssize_t offset;        // offset of the leaf node
    int place;				// place of the element in the leaf node
    const BTree *from;
  public:
    const_iterator() {
        from = nullptr;
        place = 0, offset = 0;
    }
  };



  // Default Constructor and Copy Constructor
  BTree() {
      fp_name.setName(ID);
      fp = nullptr;
      open();
      if (exist==0) initialbuild();
  }


  BTree(const BTree& other) {
      fp_name.setName(ID);
      open();
      copy(fp_name.str,other.fp_name.str);
  }


  BTree& operator=(const BTree& other) {
      fp_name.setName(ID);
      open();
      copy(fp_name.str, other.fp_name.str);
  }


  ~BTree() {
      close();
  }


  // Insert: Insert certain Key-Value into the database
  // Return a pair, the first of the pair is the iterator point to the new
  // element, the second of the pair is Success if it is successfully inserted
  pair<iterator, OperationResult> insert(const Key& key, const Value& value)
  {
      ssize_t leaf_offset = locate_leaf(key,info.root);
      leafNode leaf;

      if (info.size==0 || leaf_offset==0)
      {
          read(&leaf, info.head, 1, sizeof(leafNode));
          pair<iterator, OperationResult> ret = insert_leaf(leaf, key, value);
          if (ret.second==Fail) return ret;

          ssize_t offset = leaf.parent;
          internalNode node;
          while(offset!=0)
          {
              read(&node, offset, 1, sizeof(internalNode));
              node.key[0] = key;
              write(&node, offset, 1, sizeof(internalNode));
              offset = node.parent;
          }
          return ret;
      }

      read(&leaf,leaf_offset, 1, sizeof(leafNode));
      pair<iterator,OperationResult>ret = insert_leaf(leaf, key, value);
      return ret;
  }


  // Erase: Erase the Key-Value
  // Return Success if it is successfully erased
  // Return Fail if the key doesn't exist in the database
  OperationResult erase(const Key& key) {
      return Fail;  // If you can't finish erase part, just remaining here.
      //I can't
  }


  Value at(const Key& key)
  {
      iterator it = find(key);
      if(it==end()) throw "not found";

      leafNode leaf;
      read(&leaf, it.offset, 1, sizeof(leafNode));
      return leaf.data[it.place].second;
  }


  const Value& at(const Key& key) const
  {
      iterator it = find(key);
      if (it==end()) throw "not found";

      leafNode leaf;
      read(&leaf, it.offset, 1, sizeof(leafNode));
      return leaf.data[it.place].second;
  }


  // Return a iterator to the beginning
  iterator begin() {return iterator(this,info.head,0);}

  const_iterator cbegin() const {return const_iterator();}

  // Return a iterator to the end(the next element after the last)
  iterator end()
  {
      leafNode tail;
      read(&tail, info.tail, 1, sizeof(leafNode));
      return iterator(this, info.tail, tail.num);
  }

  const_iterator cend() const
  {
      return const_iterator();
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
      open();
      initialbuild();
  }


  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key& key) const
  {
      return static_cast<size_t>(find(key)!=end());
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
      ssize_t leaf_offset=locate_leaf(key, info.root);
      if (leaf_offset==0) return end();

      leafNode leaf;
      read(&leaf, leaf_offset, 1, sizeof(leafNode));
      for (int i=0; i<leaf.num; i++)
        if (leaf.data[i].first==key) return iterator(this,leaf_offset,i);
      return end();
  }


  const_iterator find(const Key& key) const
  {
      return const_iterator();
  }
};
}  // namespace sjtu
