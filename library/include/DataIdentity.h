/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>

#include "DataDefs.h"

/*
 * Definitions of DFHack namespace structs used by generated headers.
 */

namespace DFHack
{
    class DFHACK_EXPORT primitive_identity : public type_identity {
    public:
        primitive_identity(size_t size) : type_identity(size) {};

        virtual identity_type type() { return IDTYPE_PRIMITIVE; }
    };

    class DFHACK_EXPORT pointer_identity : public primitive_identity {
        type_identity *target;

    public:
        pointer_identity(type_identity *target = NULL)
            : primitive_identity(sizeof(void*)), target(target) {};

        virtual identity_type type() { return IDTYPE_POINTER; }

        type_identity *getTarget() { return target; }

        std::string getFullName();

        static void lua_read(lua_State *state, int fname_idx, void *ptr, type_identity *target);
        static void lua_write(lua_State *state, int fname_idx, void *ptr, type_identity *target, int val_index);

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class DFHACK_EXPORT container_identity : public constructed_identity {
        type_identity *item;
        enum_identity *ienum;

    public:
        container_identity(size_t size, TAllocateFn alloc, type_identity *item, enum_identity *ienum = NULL)
            : constructed_identity(size, alloc), item(item), ienum(ienum) {};

        virtual identity_type type() { return IDTYPE_CONTAINER; }

        std::string getFullName() { return getFullName(item); }

        virtual void build_metatable(lua_State *state);
        virtual bool isContainer() { return true; }

        type_identity *getItemType() { return item; }
        type_identity *getIndexEnumType() { return ienum; }

        virtual std::string getFullName(type_identity *item);

        int lua_item_count(lua_State *state, void *ptr);

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

    protected:
        virtual int item_count(void *ptr) = 0;
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) = 0;
    };

    class DFHACK_EXPORT ptr_container_identity : public container_identity {
    public:
        ptr_container_identity(size_t size, TAllocateFn alloc,
                               type_identity *item, enum_identity *ienum = NULL)
            : container_identity(size, alloc, item, ienum) {};

        virtual identity_type type() { return IDTYPE_PTR_CONTAINER; }

        std::string getFullName(type_identity *item);

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);
    };

    class DFHACK_EXPORT bit_container_identity : public container_identity {
    public:
        bit_container_identity(size_t size, TAllocateFn alloc, enum_identity *ienum = NULL)
            : container_identity(size, alloc, NULL, ienum) {};

        virtual identity_type type() { return IDTYPE_BIT_CONTAINER; }

        std::string getFullName(type_identity *item);

        virtual void lua_item_reference(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_read(lua_State *state, int fname_idx, void *ptr, int idx);
        virtual void lua_item_write(lua_State *state, int fname_idx, void *ptr, int idx, int val_index);

    protected:
        virtual void *item_pointer(type_identity *, void *, int) { return NULL; }

        virtual bool get_item(void *ptr, int idx) = 0;
        virtual void set_item(void *ptr, int idx, bool val) = 0;
    };
}

namespace df
{
    using DFHack::primitive_identity;
    using DFHack::pointer_identity;
    using DFHack::container_identity;
    using DFHack::ptr_container_identity;
    using DFHack::bit_container_identity;

    class number_identity_base : public primitive_identity {
        const char *name;

    public:
        number_identity_base(size_t size, const char *name)
            : primitive_identity(size), name(name) {};

        std::string getFullName() { return name; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);

    protected:
        virtual double read(void *ptr) = 0;
        virtual void write(void *ptr, double val) = 0;
    };

    template<class T>
    class number_identity : public number_identity_base {
    public:
        number_identity(const char *name) : number_identity_base(sizeof(T), name) {}
    protected:
        virtual double read(void *ptr) { return double(*(T*)ptr); }
        virtual void write(void *ptr, double val) { *(T*)ptr = T(val); }
    };

    class bool_identity : public primitive_identity {
    public:
        bool_identity() : primitive_identity(sizeof(bool)) {};

        std::string getFullName() { return "bool"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class stl_string_identity : public primitive_identity {
    public:
        stl_string_identity() : primitive_identity(sizeof(std::string)) {};

        std::string getFullName() { return "string"; }

        virtual void lua_read(lua_State *state, int fname_idx, void *ptr);
        virtual void lua_write(lua_State *state, int fname_idx, void *ptr, int val_index);
    };

    class stl_ptr_vector_identity : public ptr_container_identity {
    public:
        /*
         * This class assumes that std::vector<T*> is equivalent
         * in layout and behavior to std::vector<void*> for any T.
         */

        stl_ptr_vector_identity(type_identity *item = NULL, enum_identity *ienum = NULL)
            : ptr_container_identity(sizeof(std::vector<void*>),allocator_fn<std::vector<void*> >,item, ienum)
        {};

        std::string getFullName(type_identity *item) {
            return "vector" + ptr_container_identity::getFullName(item);
        }

        virtual DFHack::identity_type type() { return DFHack::IDTYPE_STL_PTR_VECTOR; }

    protected:
        virtual int item_count(void *ptr) {
            return ((std::vector<void*>*)ptr)->size();
        };
        virtual void *item_pointer(type_identity *, void *ptr, int idx) {
            return &(*(std::vector<void*>*)ptr)[idx];
        }
    };

    class buffer_container_identity : public container_identity {
        int size;

    public:
        buffer_container_identity()
            : container_identity(0, NULL, NULL, NULL), size(0)
        {}

        buffer_container_identity(int size, type_identity *item, enum_identity *ienum = NULL)
            : container_identity(item->byte_size()*size, NULL, item, ienum), size(size)
        {}

        std::string getFullName(type_identity *item);

        static buffer_container_identity base_instance;

    protected:
        virtual int item_count(void *ptr) { return size; }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            return ((uint8_t*)ptr) + idx * item->byte_size();
        }
    };

    template<class T>
    class stl_container_identity : public container_identity {
        const char *name;

    public:
        stl_container_identity(const char *name, type_identity *item, enum_identity *ienum = NULL)
            : container_identity(sizeof(T), &allocator_fn<T>, item, ienum), name(name)
        {}

        std::string getFullName(type_identity *item) {
            return name + container_identity::getFullName(item);
        }

    protected:
        virtual int item_count(void *ptr) { return ((T*)ptr)->size(); }
        virtual void *item_pointer(type_identity *item, void *ptr, int idx) {
            return &(*(T*)ptr)[idx];
        }
    };

    class bit_array_identity : public bit_container_identity {
    public:
        /*
         * This class assumes that BitArray<T> is equivalent
         * in layout and behavior to BitArray<int> for any T.
         */

        typedef BitArray<int> container;

        bit_array_identity(enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &allocator_fn<container>, ienum)
        {}

        std::string getFullName(type_identity *item) {
            return "BitArray<>";
        }

    protected:
        virtual int item_count(void *ptr) { return ((container*)ptr)->size * 8; }
        virtual bool get_item(void *ptr, int idx) {
            return ((container*)ptr)->is_set(idx);
        }
        virtual void set_item(void *ptr, int idx, bool val) {
            ((container*)ptr)->set(idx, val);
        }
    };

    class stl_bit_vector_identity : public bit_container_identity {
    public:
        typedef std::vector<bool> container;

        stl_bit_vector_identity(enum_identity *ienum = NULL)
            : bit_container_identity(sizeof(container), &allocator_fn<container>, ienum)
        {}

        std::string getFullName(type_identity *item) {
            return "vector" + bit_container_identity::getFullName(item);
        }

    protected:
        virtual int item_count(void *ptr) { return ((container*)ptr)->size(); }
        virtual bool get_item(void *ptr, int idx) {
            return (*(container*)ptr)[idx];
        }
        virtual void set_item(void *ptr, int idx, bool val) {
            (*(container*)ptr)[idx] = val;
        }
    };

#define NUMBER_IDENTITY_TRAITS(type) \
    template<> struct identity_traits<type> { \
        static number_identity<type> identity; \
        static number_identity_base *get() { return &identity; } \
    };

    NUMBER_IDENTITY_TRAITS(char);
    NUMBER_IDENTITY_TRAITS(int8_t);
    NUMBER_IDENTITY_TRAITS(uint8_t);
    NUMBER_IDENTITY_TRAITS(int16_t);
    NUMBER_IDENTITY_TRAITS(uint16_t);
    NUMBER_IDENTITY_TRAITS(int32_t);
    NUMBER_IDENTITY_TRAITS(uint32_t);
    NUMBER_IDENTITY_TRAITS(int64_t);
    NUMBER_IDENTITY_TRAITS(uint64_t);
    NUMBER_IDENTITY_TRAITS(float);

    template<> struct identity_traits<bool> {
        static bool_identity identity;
        static bool_identity *get() { return &identity; }
    };

    template<> struct identity_traits<std::string> {
        static stl_string_identity identity;
        static stl_string_identity *get() { return &identity; }
    };

    template<> struct identity_traits<void*> {
        static pointer_identity identity;
        static pointer_identity *get() { return &identity; }
    };

    template<> struct identity_traits<std::vector<void*> > {
        static stl_ptr_vector_identity identity;
        static stl_ptr_vector_identity *get() { return &identity; }
    };

    template<> struct identity_traits<std::vector<bool> > {
        static stl_bit_vector_identity identity;
        static stl_bit_vector_identity *get() { return &identity; }
    };

#undef NUMBER_IDENTITY_TRAITS

    // Container declarations

    template<class Enum, class FT> struct identity_traits<enum_field<Enum,FT> > {
        static primitive_identity *get();
    };

    template<class T> struct identity_traits<T *> {
        static pointer_identity *get();
    };

    template<class T, int sz> struct identity_traits<T [sz]> {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<std::vector<T*> > {
        static stl_ptr_vector_identity *get();
    };

    template<class T> struct identity_traits<std::deque<T> > {
        static container_identity *get();
    };

    template<class T> struct identity_traits<BitArray<T> > {
        static bit_container_identity *get();
    };

    template<class T> struct identity_traits<DfArray<T> > {
        static container_identity *get();
    };

    // Container definitions

    template<class Enum, class FT>
    primitive_identity *identity_traits<enum_field<Enum,FT> >::get() {
        return identity_traits<FT>::get();
    }

    template<class T>
    pointer_identity *identity_traits<T *>::get() {
        static pointer_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T, int sz>
    container_identity *identity_traits<T [sz]>::get() {
        static buffer_container_identity identity(sz, identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<std::vector<T> >::get() {
        typedef std::vector<T> container;
        static stl_container_identity<container> identity("vector", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    stl_ptr_vector_identity *identity_traits<std::vector<T*> >::get() {
        static stl_ptr_vector_identity identity(identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<std::deque<T> >::get() {
        typedef std::deque<T> container;
        static stl_container_identity<container> identity("deque", identity_traits<T>::get());
        return &identity;
    }

    template<class T>
    bit_container_identity *identity_traits<BitArray<T> >::get() {
        static type_identity *eid = identity_traits<T>::get();
        static enum_identity *reid = eid->type() == DFHack::IDTYPE_ENUM ? (enum_identity*)eid : NULL;
        static bit_array_identity identity(reid);
        return &identity;
    }

    template<class T>
    container_identity *identity_traits<DfArray<T> >::get() {
        typedef DfArray<T> container;
        static stl_container_identity<container> identity("DfArray", identity_traits<T>::get());
        return &identity;
    }
}

