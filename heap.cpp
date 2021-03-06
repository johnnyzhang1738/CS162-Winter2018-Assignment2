#include "heap.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdio.h>
#include <string.h>


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

	if (bump_ptr + size > heap_size/2){
    //RESET BUMP POINTER
    
    //bump_ptr = 0;

		collect();
	}
	//check we have space on heap AGAIN
	//if we still don't have space, we're out of memory
	if (bump_ptr + size > heap_size/2){
    //printf("OKAY WE ARE REALLY OUT\n");
		throw OutOfMemoryException(); 
	}
	//otherwise, we do have space to allocate...
	int32_t bump_before_allocation = bump_ptr;
	bump_ptr += size; // increment bump pointer by size allocated
	
	//printf("bump pointer is at %i, %i \n", bump_ptr, heap_size/2);
	return bump_before_allocation;
}

// This method should implement the actual semispace garbage collection.
// As a final result this method *MUST* call print();
void Heap::collect() {
  bump_ptr = 0;
 for (auto iter = root_set.begin(); iter != root_set.end(); ++iter){
    
    std::string var_name = iter->first;
  	obj_ptr current_obj_address = iter->second;

		iter->second = Heap::collect_helper(current_obj_address);
    //printf("placerholder");
	}

  //printf("\n");

  //printf("Bump pointer after collection: %i\n", bump_ptr);
	// at the very end, we need to swap from and to labels...
  //printf("check to/from pointers before: %p,%p\n",to,from);
	byte *tmp = from;
  from = to;
  to = tmp;

  // Please do not remove the call to print, it has to be the final
  // operation in the method for your assignment to be graded.
  print();
}

obj_ptr Heap::collect_helper(obj_ptr address_LOCAL){
  object_type *obj = global_address<object_type>(address_LOCAL);

  byte *new_loc = reinterpret_cast<byte*>(to+bump_ptr);
  byte *old_loc = reinterpret_cast<byte*>(obj);
 	
	switch(*obj){
    case FOO:{
    	//printf("this is a FOO obj\n"); 
      memcpy(new_loc, old_loc, sizeof(Foo));
      bump_ptr += sizeof(Foo);
      //foo is a pointer to an actual Foo object.
    	auto *foo = global_address<Foo>(address_LOCAL);
      foo->type = MOVED;

    	if (foo->c != nil_ptr){
    		foo->c = collect_helper(foo->c);
    	}
      if (foo->d != nil_ptr){
        foo->d = collect_helper(foo->d);
      }

      break;
    }
    case BAR:{
      //printf("this is a BAR obj\n");
      memcpy(new_loc, old_loc, sizeof(Bar));
      bump_ptr += sizeof(Bar);

      auto *bar = global_address<Bar>(address_LOCAL);
      bar->type = MOVED;

    	if (bar->c != nil_ptr){
    		bar->c = collect_helper(bar->c);
    	}
      
      if (bar->f != nil_ptr){
        bar->f = collect_helper(bar->f);
      }
      break;
    }
    case BAZ:{
     // printf("this is a BAZ obj\n");
      memcpy(new_loc, old_loc, sizeof(Baz));
      bump_ptr += sizeof(Baz);

      auto *baz = global_address<Baz>(address_LOCAL);
      baz->type = MOVED;

    	if (baz->b != nil_ptr){
    		baz->b = collect_helper(baz->b);
    	}
      
      if (baz->c != nil_ptr){
        baz->c = collect_helper(baz->c);
      }
      break;
    }
    case MOVED:{
      //byte *already_in_from = reinterpret_cast<byte*>(to+obj);
      //return local_address(already_in_from);
      return local_address(old_loc);
    }
  }
	
  //printf("\n");
  return local_address(old_loc);
}

obj_ptr Heap::get_root(const std::string& name) {
  auto root = root_set.find(name);
  if(root == root_set.end()) {
    throw std::runtime_error("No such root: " + name);
  }

  return root->second;
}

object_type Heap::get_object_type(obj_ptr ptr) {
  return *reinterpret_cast<object_type*>(from + ptr);
}

// Finds fields by path / name; used by get() and set().
obj_ptr *Heap::get_nested(const std::vector<std::string>& path) {
  std::string current = path.at(0);
  obj_ptr init = get_root(path[0]);
  obj_ptr *fld = &init;

  for(size_t i = 1; i < path.size(); ++i) {
    if(*fld == nil_ptr) {
      std::string message("Nil pointer while getting: ");
      throw std::runtime_error(message + current);
    }

    auto addr = *fld;
    auto type = get_object_type(addr);
    auto seg  = path[i];
    current  += "." + seg;

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
    }
    default: {
      std::string message("Unknown object type while getting: ");
      throw std::runtime_error(message + std::to_string(int(type)));
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
  obj_ptr position = 0;
  typedef std::pair<obj_ptr,const char*> pair;
  std::vector<pair> objects;

  while(position < heap_size / 2 && position < bump_ptr) {
    object_type type = get_object_type(position);
    switch(type) {
      case FOO: {
        auto obj = global_address<Foo>(position);
        objects.push_back(pair(obj->id, "Foo"));
        position += sizeof(Foo);
        break;
      }
      case BAR: {
        auto obj = global_address<Bar>(position);
        objects.push_back(pair(obj->id, "Bar"));
        position += sizeof(Bar);
        break;
      }
      case BAZ: {
        auto obj = global_address<Baz>(position);
        objects.push_back(pair(obj->id, "Baz"));
        position += sizeof(Baz);
        break;
      }
      default: {
        std::string message("Unknown object type while printing: ");
        throw std::runtime_error(message + std::to_string(int(type)));
      }
    }
  }

  std::cout << "Objects in from-space:\n";
  std::sort(objects.begin(), objects.end());
  for(auto const& itr: objects) {
    std::cout << " - " << itr.first << ':' << itr.second << '\n';
  }
}