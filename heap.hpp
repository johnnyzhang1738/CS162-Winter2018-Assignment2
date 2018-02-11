#include <cstdint>
#include <stdexcept>
#include <string>
#include <map>
#include <vector>

typedef int8_t byte;
typedef int32_t obj_ptr;

// All addresses on the heap are relative to the `from` pointer.
// For example, allocating an object on an empty heap will give it
// an address 0. Because of this, we use a special constant nil_ptr
// to indicate nil pointers. 
//
static const obj_ptr nil_ptr(-1);

// The enum below will always be the first member of all structs
// that can be allocated on the heap. It tags the object with its
// type, making it possible to easily cast a byte pointer to a 
// pointer to an appropriate struct. This is also useful
// when calculating struct sizes and offsets.
// The object type tag will always be a single byte in size.
// 
enum object_type : byte {
  FOO, BAR, BAZ, MOVED
};

// There are three possible objects we can allocate on the heap,
// Foo, Bar and Baz. Each contain the following:
// - type (an object_type tag)
// - id a unique object id, used to print all currently live objects
//   and their types
// - pointers to other objects, of type obj_ptr
// - non-pointer data, which will always be 0 and is of type `byte`
//
// So for example, the struct Foo has c and d as its pointer members,
// and a and b as data members. You *do not* have to do anything with
// the data members, they only exist to model a realistic looking struct.

struct Foo {
  object_type type = FOO;
  uint32_t   id;

  byte       a = 0;
  byte       b = 0;
  obj_ptr    c = nil_ptr;
  obj_ptr    d = nil_ptr;

  Foo(uint32_t id) : id(id) {}
};

struct Bar {
  object_type type = BAR;
  uint32_t   id;

  byte       a = 0;
  byte       b = 0;
  obj_ptr    c = nil_ptr;
  byte       d = 0;
  byte       e = 0;
  obj_ptr    f = nil_ptr;

  Bar(uint32_t id) : id(id) {}
};

struct Baz {
  object_type type = BAZ;
  uint32_t   id;

  byte       a = 0;
  obj_ptr    b = nil_ptr;
  obj_ptr    c = nil_ptr;

  Baz(uint32_t id) : id(id) {}
};

// This exception should be thrown when an allocation is attempted, but
// garbage collection is unable to free the needed space.
// For example, if we try to allocate 20 bytes, but have only 10 free
// even after running garbage collection, the implementation should
// throw this exception.
//
struct OutOfMemoryException : public std::runtime_error {
  OutOfMemoryException() : std::runtime_error("Out of memory") {}
};

// This is your heap class, it contains everything you need to implement
// garbage collection. You should only implement the following methods:
// - allocate(int32_t size)
// - collect()
// Optionally, you might want to implement dump_heap for debugging purposes.
// This method should be used to dump the state of the heap and help you
// debug problems in your implementation.
class Heap {
  // heap is the pointer to the actual memory buffer we allocate objects in
  byte *heap;
  // from points to the beginning of the from-space
  byte *from;
  // to points to the beginning of the to-space
  byte *to;
  // heap_size is the size of the whole heap, and will always be an even number
  int32_t heap_size;
  // bump_ptr is the bump pointer in the from space, relative to the from pointer.
  // This means that it will initially be 0, and is incremented as allocation happens.
  // If we were to allocate 10 bytes on an empty heap, bump_ptr would then point to 10.
  int32_t bump_ptr;

  // This is your root set, it maps variables to object pointers (again, object pointers
  // are relative to the from pointer). If you were to allocate objects of sizes 5 and 10
  // on an empty heap, and assign them to x and y, your root set would look like this:
  // x -> 0
  // y -> 10
  std::map<std::string, obj_ptr> root_set;

  
  // This is the object id counter, it assigns every new object a unique id.
  // You don't need to touch this in any way.
  uint32_t object_id = 0;

public:
  Heap(int32_t heap_size);
  ~Heap();

  // You should define allocate in impl.cpp
  obj_ptr allocate(int32_t size);

  // You should define collect in impl.cpp
  void    collect();
  obj_ptr collect_helper(obj_ptr address_LOCAL);
  // You should optionally define debug in impl.cpp
  void    debug();

  // This method will dump (print) all objects currently in the from space.
  // Do *not* alter this, as it is used for grading. It can also help you
  // debug your implementation.
  void    print();


  // The following three methods create instances of Foo, Bar and Baz respectively.
  // In the background, they will call your allocate() method, and will properly
  // initialize the memory. You only need to worry about memory allocation.
  obj_ptr new_foo();
  obj_ptr new_bar();
  obj_ptr new_baz();

  // get(path) returns a pointer to the object by following a path from the root
  // set. For example, if you have:
  // x = Foo
  // x.c = Bar
  // x.c.f = Baz
  // then calling get("x.c.f") will return the address of the Baz object.
  // If the object can not be found or the path is invalid, an exception is thrown.
  obj_ptr  get(const std::vector<std::string>& path);

  // set(path, obj_ptr) will assign an object pointer to a path.
  // In the previous example, doing set("x.c.f", nil_ptr) will set
  // the Bar object's pointer 'f' to nil.
  void     set(const std::vector<std::string>& path, obj_ptr value);

  // The following two methods are auxiliary, you should use get() instead.
  obj_ptr* get_nested(const std::vector<std::string>& path);
  obj_ptr  get_root(const std::string& name);

  // This is a convenience method that, given a pointer of an object that resides
  // in the from space, will return its type (ie. FOO, BAR or BAZ).
  object_type get_object_type(obj_ptr);

  // Converts a global pointer (to an actual struct) to a heap-local pointer.
  // For example, if you have an object Foo at the beginning of the heap,
  // and would like its local address (which in this case would be 0), you can do:
  // obj_ptr p = local_address(heap)
  //
  template<class T>
  int32_t local_address(T *obj) {
    return (int32_t)(reinterpret_cast<byte*>(obj) - from);
  }

  // This function converts a local address (an obj_ptr) to an actual
  // pointer to the struct on the heap. If you have an object at address 12
  // in the heap, and you know it to be of type Foo, then doing:
  //   global_address<Foo>(12)
  // will return a Foo* pointer, that can be used to read or modify the
  // actual structure.
  template<class T>
  T* global_address(int32_t addr) {
    return reinterpret_cast<T*>(from + addr);
  }
};
