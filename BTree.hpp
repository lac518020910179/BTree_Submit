#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
namespace sjtu {
const int maxn=4096;
int ID=0;
template <class Keyt, class Valuet, class Compare = std::less<Keyt> >
class BTree {
    typedef pair <Keyt, Valuet> value_type;
 private:
    static const int M=maxn/sizeof(Keyt);//zhenzhuoxia
    static const int L=sizeof(value_type)>maxn?1:maxn/sizeof(value_type);//zhenzhuoxia
    static const int MINM=(M+1)/2;
    static const int MINL=(L+1)/2；
    static const int bpt_info_offset=0;

    struct FileName
    {
        char str;
        FileName(){str=new char[10];}
        ~FileName(if(str!=NULL)delete str;)
        void setname(int id)
        {
            if(id<0||id>9)throw "no more B plus Tree!";
            str[0]='d';
            str[1]='a';
            str[2]='t';
            str[3]=static_cast <char> (id + '0');
            str[4]='.';
            str[5]='d';
            str[6]='a';
            str[7]='t';
            str[8]='\0';
        }
        void setname(char *other)
        {
            int i=0;
            for(;str[i];++i)str[i]=other[i];
            str[i]=0;
        }

    };
    struct bpt_Info
    {
        ssize_t root;//bpt root
        ssize_t head;//leaf head
        ssize_t tail;//leaf tail
        size_t size;//whole bpt
        ssize_t eof;//end of file

        bpt_Info()
        {
            root=0;
            haed=0;
            tail=0;
            size=0;
            eof=0;
        }
    };
    struct leafnode
    {
        ssize_t offset;//偏移量
        ssize_t father;//爸爸节点
        ssize_t pre,next;//前置和后置
        int num;//num o pairs
        value_type data[L+1];
        leafnode()
        {
            offset=0;
            father=0;
            pre=0;
            next=0;
            num=0;
        }
    };

    struct internalnode
    {
        ssize_t offset;
        ssize_t father;
        ssize_t child[M+1];
        Keyt key[M+1];
        int num;//num o key
        bool leafornot;//0-noop 1-yeph child is leaf or not
        internalnode()
        {
            offset=0;
            father=0;
            for(int i=0;i<M+1;i++)child[i]=0;
            num=0;
            leafornot=0;
        }
    }
    FILE *fp;
    bool fp_open;
    FileName fp_name;
    bpt_Info info;
    bool exist;

    //file operation

    void openfile()
    {
        exist=1;
        if(!fp_open)
        {
            fp=fopen(fp_name.str,"rb+");
            if(fp==NULL)
            {
                exist=0;
                fp=fopen(fp_name.str,"w");
                fclose(fp);
                fp=fopen(fp_name.str,"rb+")
            }
            else read(&info,bpt_offset, 1, sizeof(bpt_Info));
            fopen=1;
        }
    }

    void closefile()
    {
        if(fp_open){fclose(fp);fp_open=0;}
    }

    void read(void* place,ssize_t offset,size_t num,size_t size)
    {
         /**

             *  int fseek(FILE *stream, long offset, int fromwhere)

             *  If the execution succeeds, the stream points to a position

             *  with fromwhere as the base, offset offset (pointer offset) bytes,

             *  and the function returns 0.

             *  If execution fails, the position to which the stream points

             *  is not changed, and the function returns a non-0 value.

             */



            /**

             *  size_t fread ( void *buffer, size_t size, size_t count, FILE *stream) ;

             *  this function return the true number of read, if it doesn't match count ,

             *  there must be some errors...

             *  Buffer : Memory address used to receive data

             *  Size : The number of bytes per data item to read, in bytes

             *  Count : To read the Count data item, each data item is a size byte.

             *  Stream : Input stream

             */
        if(fseek(fp,offset,SEEK_SET))return;
        fread(place,size,num,fp);
    }

    void write(void* place,ssize_t offset,size_t num,size_t size)
    {
        /**

             * Size_t fwrite (const void buffer, size_t size, size_t count, FILE Stream);

             * Return value: Returns the number of blocks of data actually written

             * Buffer: is a pointer, for Fwrite, is to get the address of the data;

             * Size: The number of single bytes to write to the content;

             * Count: The number of data items to be written to the size byte;

             * Stream: target file pointer;

             * Returns the count of the number of data items actually written.

             */
        if(fseek(fp,offset,SEEK_SET)) return;
        fwrite(place,size,num,fp);
    }

    void initialbuild()
    {
        info.size=0;
        info.eof=bpt_info_offset;
        info.eof+=sizeof(bpt_Info);
        internalnode root;
        leafnode leaf;
        info.root=root.offset=info.eof;
        info.eof+=sizeof(internalnode);
        info. head=info.tail=leaf.offset=info.eof;
        info.eof+=sizeof(leafnode);
        root.father=0;
        root.num=1;
        root.leafornot=1;
        root.chidren[0]=leaf.offset;
        leaf.father=root.offset;
        leaf.pre=leaf.next=leaf.num=0;
        write(&info,bpt_info_offset,1,sizeof(bpt_Info));
        write(&root,root.offset,1,sizeof(internalnode));
        write(&leaf,leaf.offset,1,sizeof(leafnode));
    }

    ssize_t leaflocate(const Keyt key,ssize_t offset)
    {
        internalnode tem;
        read(&tem,offset,1,sizeof(internalnode));
        if(tem.leafornot)
        {
            int pos=0;
            for(;pos<tem.num&&key>=tem.key[pos];pos++);
            if(pos==0)return 0;
            return tem.child[pos-1];
        }
        else
        {
            int pos=0;
            for(;pos<tem.num&&key>=tem.key[pos];pos++);
            if(pos==0)return 0;
            return leaflocate(key,tem.child[pos-1]);
        }
    }

    std::pair<iterator,OperationResult> insert_leaf(leafnode leaf,const Keyt key,const Valuet value)
    {
        int pos=0;
        for(;pos<leaf.num&&key>leaf.data[pos].first;pos++)if(key==leaf.data[pos].first)return pair<iterator,OperationResult>(iterator(), Fail);
        for(int i = leaf.num - 1;i >= pos;i--)//houyi
        {
            leaf.data[i + 1].first = leaf.data[i].first;
            leaf.data[i + 1].second = leaf.data[i].second;
        }
        leaf.data[pos].first=key;
        leaf.data[pos].second=value;
        leaf.num++;
        info.size++;
        write(&info,bpt_info_offset,1,sizeof(bpt_Info));
        if(leaf.num<=L-1)
        write(&leaf,leaf.offset,1,sizeof(leafnode));
        else split_leaf(leaf,key);
        return std::pair<iterator,OperationResult>(iterator(),Success);
    }

    void insert_node(internalnode &internal,const Keyt &key,const ssize_t child)
    {
        int pos=1;
        for(;pos<=internal.num&&key>=internal.key[pos];pos++);
        for(int i=internal.num;i>=pos;i--){internal.key[i+1]=internal.key[i];internal.child[i+1]=inter.child[i];}        
        internal.key[pos]=key;
        internal.child[pos]=child;
        internal.num++;
        if(internal.num<M-1)write(&internal,internal.offset,1,sizeof(internalnode));
        else split_node(internal);
        
    }
    
    void split_leaf(leafnode &leaf,const Key &key)
    {
        leafnode another;
        another.num=leaf.num-(leaf.num/2);
        leaf.num/=2;
        another.offset=info.eof;
        another.offset=info.eof;
        another.father=info.father;
        another.next=leaf.next;
        another.pre=leaf.offset;
        leaf.next=another.offset;
        info.eof+=sizeof(leafnode);
        for(int i=0;i<another.num;i++)
        {
            another.data[i].first=leaf.data[i+leaf.num].first;
            another.data[i].second=leaf.data[i+leaf.num].second;
        }
        if(another.next==0)info.tail=another.offset;
        else
        {
            leafnode next_leaf;
            read(&next_leaf,another.next,1,sizeof(leafnode));
            next_leaf.pre=another.offset;
            write(&next_leaf,another.next,1,sizeof(leafnode));
        }
        
        write(&info,bpt_info_offset,1,sizeof(bpt_Info));
        write(&leaf,leaf.offset,1,sizeof(leafnode));
        write(&another,another.offset,1,sizeof(leafnode));
        internalnode father;
        read(&father,leaf.father,1,sizeof(internalnode));
        insert_node(father,another.data[0].first,another.offset);
    }
    
    void split_node(internalnode &internal)
    {
        if(internal.offset!=info.root)
        {
            internalnode another;
            another.offset=info.eof;
            info.eof+=sizeof(internalnode);
            another.num=internal.num-(internal.num/2)-1;
            internal.num/=2;
            another.father=internal.father;
            another.type=internal.type;
            for(int i=0;i<=another.num;i++)
            {
                another.child[i]=internal.child[i+1+internal.num];
            }
            for(int i=1;i<=another.num;i++)
            {
                another.key[i]=internal.key[i+1+internal.num];
            }
            write(&internal,internal.offset,1,sizeof(internalnode));
            write(&another,another.offset,1,sizeof(internalnode));
            
            internalnode tem;
            leafnode leaf;
            for(int i=0;i<=another;i++)
            {
                if(another.type==1)
                {
                    read(&leaf,another.child[i],1,sizeof(leafnode));
                    leaf.father=another.offset;
                    write(&leaf,another.child[i],1,sizeof(leafnode));
                }
                else
                {
                    read(&tem,another.child[i],1,sizeof(internalnode));
                    tem.father=another.offset;
                    write(&tem,another.child[i],1,sizeof(internalnode));
                }
            }
            read(&tem,internal.father,1,sizeof(internalnode));
            insert_node(tem,internal.key[internal.nu+1],another.offset);
            
        }
        else
        {
            internalnode new_root;
            internalnode half;
            new_root.offset=info.eof;
            info.eof+=sizeof(internalnode);
            half.offset=info.eof;
            info.eof+=sizeof(internalnode);
            info.root=new_root.offset;
            internal.father=info.root;
            half.father=info.root;
            half.num=internal.num-(internal.num/2)-1;
            for(int i=0;i<=half.num;i++)
            {
                half.child[i]=internal.child[i+internal.num/2+1];
            }
            for(int i=1;i<=half.num;i++)
            {
                half.key[i]=internal.key[i+internal.num/2+1];
            }
            new_root.num=1;
            new_root.father=0;
            new_root.t=0;
            half.type=internal.type;
            new_root.child[0]=internal.offset;
            new_root.child[1]=half.offset;
            new_root.key[1]=internal.key[internal.num/2+1];
            internal.num/=2;
            if(internal.type)
            {
                leafnode change;
                for(int i=0;i<=half.num;i++)
                {
                    read(&change,internal.child[i+internal.num+1],1,sizeof(leafnode));
                    change.father=half.offset;
                    write(&change,internal.child[i+internal.num+1],1,sizeof(leafnode));
                }
            }
            else
            {
                internalnode change;
                for(int i=0;i<=half.num;i++)
                {
                    read(&change,internal.child[i+internal.num+1],1,sizeof(internalnode));
                    change.father=half.offset;
                    write(&change,internal.child[i+internal.num+1],1,sizeof(internalnode));
                }
            }
            write(&info,bpt_info_offset,1,sizeof(bpt_Info));
            write(&internal,internal.offset,1,sizeof(internalnode));
            write(&new_root,new_root.offset,1,sizeof(internalnode));
            write(&half,half.offset,1,sizeof(internalnode));
            return;
        }
        
    }

 public:
    class iterator;
    class const_iterator;
    

  // Default Constructor and Copy Constructor
  BTree() {
      fp_name.setname(ID);
      fp=nullptr;
      fp_open=false;
      openfile();
      initialbuild();
      
    // Todo Default
  }
  BTree(const BTree& other) {
    // Todo Copy
  }
  BTree& operator=(const BTree& other) {
    // Todo Assignment
  }
  ~BTree() {
      closefile();
    // Todo Destructor
  }
  // Insert: Insert certain Key-Value into the database
  // Return a pair, the first of the pair is the iterator point to the new
  // element, the second of the pair is Success if it is successfully inserted
  pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
      open();
      ssize_t offset=leaflocate(key,info.root);
      leafnode leaf;
      read(&leaf,offset,1,sizeof(leafnode));
      std::pair<iterator, OperationResult> tem;
      tem=insert_leaf(leaf,key,value);
      pair<iterator,OperationResult> result;
      result.first=tem.first;
      result.second=tem.second;
      return result;
      
      
    // TODO insert function
  }
  // Erase: Erase the Key-Value
  // Return Success if it is successfully erased
  // Return Fail if the key doesn't exist in the database
  OperationResult erase(const Key& key) {
    // TODO erase function
    return Success;  // If you can't finish erase part, just remaining here.
  }
  // Overloaded of []
  // Access Specified Element
  // return a reference to the first value that is mapped to a key equivalent to
  // key. Perform an insertion if such key does not exist.
  //Value& operator[](const Key& key) {}
  // Overloaded of const []
  // Access Specified Element
  // return a reference to the first value that is mapped to a key equivalent to
  // key. Throw an exception if the key does not exist.
  //const Value& operator[](const Key& key) const {}
  // Access Specified Element
  // return a reference to the first value that is mapped to a key equivalent to
  // key. Throw an exception if the key does not exist
  //Value& at(const Key& key) {}
  // Overloaded of const []
  // Access Specified Element
  // return a reference to the first value that is mapped to a key equivalent to
  // key. Throw an exception if the key does not exist.
  //const Value& at(const Key& key) const {}
  // Return a iterator to the beginning
  iterator begin() {return iterator();}
  const_iterator cbegin() const {return const_iterator();}
  // Return a iterator to the end(the next element after the last)
  iterator end() {return iterator();}
  const_iterator cend() const {return const_iterator();}
  // Check whether this BTree is empty
  bool empty() const {return info.size==0;}
  // Return the number of <K,V> pairs
  size_t size() const {return info.size;}
  // Clear the BTree
  void clear() 
  {
      fp=fopen(fp_name.str,"w");
      fclose(fp);
      initialbuild();
  }
  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   * The default method of check the equivalence is !(a < b || b > a)
   */
  //size_t count(const Key& key) const {}
  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is
   * returned.
   */
  iterator find(const Key& key) {return iterrator();}
  const_iterator find(const Key& key) const {return const_iterator();}
};
}  // namespace sjtu
