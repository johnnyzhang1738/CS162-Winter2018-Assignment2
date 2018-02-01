#include "heap.hpp"

#include <iostream>
#include <map>


Heap::Heap(int32_t heap_size) : heap_size(heap_size), root_set() {
  heap = new byte[heap_size];
  from = heap;
  to = heap + heap_size / 2;
  bump_ptr = 0;
}

Heap::~Heap() {
  delete[] heap;
}

// This method should print out the state of the heap.
// It's not mandatory to implement it, but would be a very useful
// debugging tool. It's called whenever the DEBUG command is found
// in the input program.
void Heap::debug() {
  // Implement me
}

// The allocate method allocates a chunk of memory of given size.
// It returns an object pointer, which is local to the from space.
// For example, the first allocated object would have the address 0.
// IMPORTANT: This method should initiate garbage collection when there is not
// enough memory. If there is still insufficient memory after garbage collection,
// this method should throw an out_of_memory exception.
obj_ptr Heap::allocate(int32_t size) {
    // check we have space on heap
	//if we don't, call collect()
	int32_t triggered_point = heap_size/2;
	if (bump_ptr + size > triggered_point){
		collect();
	}
	//check we have space on heap AGAIN
	//if we still don't have space, we're out of memory
	if (bump_ptr + size > triggered_point){
		throw OutOfMemoryException(); 
	}
	//otherwise, we do have space to allocate...
	int32_t bump_before_allocation = bump_ptr;
	bump_ptr += size; // increment bump pointer by size allocated
	
	
	return bump_before_allocation;
}

// This method should implement the actual semispace garbage collection.
// As a final result this method *MUST* call print();
void Heap::collect() {
  int32_t current_mem = from;
  for (std::map<std::string, obj_ptr>::iterator iter = root_set.begin(); iter != root_set.end(); ++iter){
  	std::string var_name = iter->first;
  	current_obj_address = iter->second;
  	if (current_obj_address > heap_size/2){
  		break; // stop when obj_pointer has a memory address greater than halfway point
  	}
  	



  }
  
	// at the very end, we need to swap from and to labels...
  /*
	int32_t temp = from;
	from = to;
	to = temp;
  */
  // Please do not remove the call to print, it has to be the final
  // operation in the method for your assignment to be graded.
  print();
}

obj_ptr Heap::get_root(const std::string& name) {
  auto root = root_set.find(name);
  if(root == root_set.end()) {
    throw std::runtime_error("No such root: " + name);
  }

  return root->second;
}

// Finds fields by path / name; used by get() and set().
obj_ptr *Heap::get_nested(const std::vector<std::string>& path) {
  obj_ptr init = get_root(path[0]);
  obj_ptr *fld = &init;

  for(int i = 1; i < path.size(); ++i) {
    auto addr = *fld;
    auto type = *reinterpret_cast<object_type*>(global_address<object_type>(addr));
    auto seg  = path[i];

    switch(type) {
    case FOO: {
      auto *foo = global_address<Foo>(addr);
      if(seg == "c") fld = &foo->c;
      else if(seg == "d") fld = &foo->d;
      else throw std::runtime_error("No such field: Foo." + seg);
      break;
    }
    case BAR: {
      auto *bar = global_address<Bar>(addr);
      if(seg == "c") fld = &bar->c;
      else if(seg == "f") fld = &bar->f;
      else throw std::runtime_error("No such field: Bar." + seg);
      break;
    }
    case BAZ: {
      auto *baz = global_address<Baz>(addr);
      if(seg == "b") fld = &baz->b;
      else if(seg == "c") fld = &baz->c;
      else throw std::runtime_error("No such field: Baz." + seg);
      break;
    }}
  }

  return fld;
}

obj_ptr Heap::get(const std::vector<std::string>& path) {
  if(path.size() == 1) {
    return get_root(path[0]);
  }
  else {
    return *get_nested(path);
  }
}

void Heap::set(const std::vector<std::string>& path, obj_ptr value) {
  if(path.size() == 1) {
    if(value < 0) root_set.erase(path[0]);
    else root_set[path[0]] = value;
  }
  else {
    *get_nested(path) = value;
  }
}

obj_ptr Heap::new_foo() {
  auto heap_addr = allocate(sizeof(Foo));
  new (from + heap_addr) Foo(object_id++);
  return heap_addr;
}

obj_ptr Heap::new_bar() {
  auto heap_addr = allocate(sizeof(Bar));
  new (from + heap_addr) Bar(object_id++);
  return heap_addr;
}

obj_ptr Heap::new_baz() {
  auto heap_addr = allocate(sizeof(Baz));
  new (from + heap_addr) Baz(object_id++);
  return heap_addr;
}

void Heap::print() {
  byte *position = from;
  std::map<int32_t, const char*> objects;

  while(position < (from + heap_size / 2) && position < (from + bump_ptr)) {
    object_type type = *reinterpret_cast<object_type*>(position);
    switch(type) {
      case FOO: {
        auto obj = reinterpret_cast<Foo*>(position);
        objects[obj->id] = "Foo";
        position += sizeof(Foo);
        break;
      }
      case BAR: {
        auto obj = reinterpret_cast<Bar*>(position);
        objects[obj->id] = "Bar";
        position += sizeof(Bar);
        break;
      }
      case BAZ: {
        auto obj = reinterpret_cast<Baz*>(position);
        objects[obj->id] = "Baz";
        position += sizeof(Baz);
        break;
      }
    }
  }

  std::cout << "Objects in from-space:\n";
  for(auto const& itr: objects) {
    std::cout << " - " << itr.first << ':' << itr.second << '\n';
  }
}
