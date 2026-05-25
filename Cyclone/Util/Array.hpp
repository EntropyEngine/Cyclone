#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <concepts>
#include <ranges>
#include <cassert>
#include <initializer_list>

namespace Cyclone::Util
{
	template <typename T, typename Allocator = std::allocator<T>>
	class Array : private Allocator
	{
	public:
		using value_type				= T;
		using allocator_type			= Allocator;
		using size_type					= uint32_t;
		using difference_type			= std::ptrdiff_t;
		using pointer					= T*;
		using const_pointer				= const T*;
		using reference					= T&;
		using const_reference			= const T&;

		using iterator					= T*;
		using const_iterator			= const T*;

		using reverse_iterator			= std::reverse_iterator<iterator>;
		using const_reverse_iterator	= std::reverse_iterator<const_iterator>;

		using alloc_traits				= std::allocator_traits<allocator_type>;

	private:
		void move( pointer inDestination, pointer inSource, size_type inCount )
		{
			// Trivial fast path
			if constexpr ( std::is_trivially_copyable_v<T> )
			{
				std::memmove( inDestination, inSource, inCount * sizeof( T ) );
			}

			// Allocator path
			else
			{
				if ( inDestination < inSource )
				{
					for ( T* src = inSource, *dst = inDestination, *end = inSource + inCount; src < end; ++src, ++dst )
					{
						alloc_traits::construct( get_allocator(), dst, std::move( *src ) );
						alloc_traits::destroy( get_allocator(), src );
					}
				}
				else
				{
					for ( T* src = inSource + inCount - 1, *dst = inDestination + inCount - 1; src >= inSource; --src, --dst )
					{
						alloc_traits::construct( get_allocator(), dst, std::move( *src ) );
						alloc_traits::destroy( get_allocator(), src );
					}
				}
			}
		}

		/*
		static void relocate_into_uninitialized( allocator_type &inAlloc, pointer inDst, pointer inSrc, size_type inCount )
		{
			if constexpr ( std::is_trivially_copyable_v<T> )
			{
				std::memmove( inDst, inSrc, inCount * sizeof( T ) );
			}
			else
			{
				pointer current = inDst;

				try
				{
					for ( size_type i = 0; i < inCount; ++i, ++current )
					{
						alloc_traits::construct( inAlloc, current, std::move_if_noexcept( inSrc[i] ) );
					}
				}
				catch ( ... )
				{
					for ( pointer p = inDst; p != current; ++p )
					{
						alloc_traits::destroy( inAlloc, p );
					}

					throw;
				}

				for ( size_type i = 0; i < inCount; ++i )
				{
					alloc_traits::destroy( inAlloc, inSrc + i );
				}
			}
		}

		static void move_backward_into_uninitialized( allocator_type &inAlloc, pointer inDst, pointer inSrc, size_type inCount )
		{
			for ( size_type i = inCount; i > 0; --i )
			{
				alloc_traits::construct( inAlloc, inDst + i - 1, std::move( inSrc[i - 1] ) );
				alloc_traits::destroy( inAlloc, inSrc + i - 1 );
			}
		}

		static void move_forward_into_uninitialized( allocator_type &inAlloc, pointer inDst, pointer inSrc, size_type inCount )
		{
			for ( size_type i = 0; i < inCount; --i )
			{
				alloc_traits::construct( inAlloc, inDst + i, std::move( inSrc[i] ) );
				alloc_traits::destroy( inAlloc, inSrc + i );
			}
		}
		*/

		void reallocate( size_type inNewCapacity )
		{
			assert( inNewCapacity > 0 && inNewCapacity >= mSize );

			pointer newElements = alloc_traits::allocate( get_allocator(), inNewCapacity );

			size_type constructed = 0;

			try
			{
				// Trivial relocation fast path
				if constexpr ( std::is_trivially_copyable_v<T> )
				{
					std::memcpy( newElements, mElements, mSize * sizeof( T ) ); // TODO: memmove?
					constructed = mSize;
				}

				// Move if noexcept OR copying is impossible
				else if constexpr ( std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T> )
				{
					for ( ; constructed < mSize; ++constructed )
					{
						alloc_traits::construct( get_allocator(), newElements + constructed, std::move( mElements[constructed] ) );
					}
				}

				// Otherwise copy for strong guarantee
				else
				{
					for ( ; constructed < mSize; ++constructed )
					{
						alloc_traits::construct( get_allocator(), newElements + constructed, mElements[constructed] );
					}
				}
			}
			catch ( ... )
			{
				// Destroy partially constructed new objects
				for ( size_type i = 0; i < constructed; ++i )
				{
					alloc_traits::destroy( get_allocator(), newElements + i );
				}

				alloc_traits::deallocate( get_allocator(), newElements, inNewCapacity );

				throw;
			}

			// Destroy old objects
			if constexpr ( !std::is_trivially_destructible_v<T> )
			{
				for ( size_type i = 0; i < mSize; ++i )
				{
					alloc_traits::destroy( get_allocator(), mElements + i );
				}
			}

			// Free old storage
			if ( mElements != nullptr )
			{
				alloc_traits::deallocate( get_allocator(), mElements, mCapacity );
			}

			mElements = newElements;
			mCapacity = inNewCapacity;
		}

		void destruct( size_type inStart, size_type inEnd )
		{
			if constexpr ( !std::is_trivially_destructible_v<T> ) {
				for ( size_type i = inStart; i < inEnd; ++i ) {
					alloc_traits::destroy( get_allocator(), mElements + i );
				}
			}
		}

		void grow( size_type inAmount = 1 )
		{
			const size_type minSize = mSize + inAmount;

			// Overflow check - mSize + inAmount wraps silently on uint32_t
			assert( minSize >= mSize && "Array size overflow" );
			//assert( minSize <= std::numeric_limits<uint32_t>::max() && "Array size exceeds uint32_t capacity" );

			if ( minSize > mCapacity )
			{
				// 1.5x growth
				const size_type newCapacity = std::max( minSize, mCapacity + mCapacity / 2 );
				reserve( newCapacity );
			}
		}

		void deallocate()
		{
			if ( mElements != nullptr )
			{
				alloc_traits::deallocate( get_allocator(), mElements, mCapacity );
				mElements = nullptr;
				mCapacity = 0;
			}
		}

		void destroy()
		{
			clear();      // destroys elements, sets mSize = 0
			deallocate(); // frees buffer, sets mElements = nullptr, mCapacity = 0
		}

	public:
		// construct/copy/destroy

		// Default constructor
		Array() noexcept = default;

		// TODO is needed?
		//Array() noexcept : Array( Allocator() ) {}

		// Allocator-aware default constructor
		explicit Array( const Allocator &inAllocator ) noexcept :
			Allocator( inAllocator )
		{}

		// Size constructor
		explicit Array( size_type inLength, const Allocator &inAllocator = Allocator{} ) :
			Allocator( inAllocator )
		{
			resize( inLength );
		}

		// Size constructor with fill value
		Array( size_type inLength, const T &inValue, const Allocator &inAllocator = Allocator{} ) :
			Allocator( inAllocator )
		{
			resize( inLength, inValue );
		}

		// Initializer list constructor
		Array( std::initializer_list<T> inList, const Allocator &inAllocator = Allocator{} ) :
			Allocator( inAllocator )
		{
			assign( inList );
		}

		// Iterator range constructor
		template <typename Iterator>
		Array( Iterator inBegin, Iterator inEnd, const Allocator &inAllocator = Allocator{} ) :
			Allocator( inAllocator )
		{
			assign( inBegin, inEnd );
		}

		// Copy constructor - informs allocator that the container is being copied if necessary
		Array( const Array &inRHS ) :
			Allocator( alloc_traits::select_on_container_copy_construction( inRHS.get_allocator() ) )
		{
			assign( inRHS.begin(), inRHS.end() );
		}

		// Copy constructor - with explict allocator override
		Array( const Array &inRHS, const Allocator &inAllocator ) : // TODO: type_identity_t?
			Allocator( inAllocator )
		{
			assign( inRHS.begin(), inRHS.end() );
		}

		// Move construct - take full ownership from source
		Array( Array &&inRHS ) noexcept( alloc_traits::is_always_equal::value && std::is_nothrow_move_constructible_v<Allocator> ) :
			Allocator( std::move( inRHS.get_allocator() ) ),
			mElements( inRHS.mElements ),
			mSize( inRHS.mSize ),
			mCapacity( inRHS.mCapacity )
		{
			inRHS.mElements = nullptr;
			inRHS.mSize = 0;
			inRHS.mCapacity = 0;
		}

		// Move constructor - with explict allocator override
		Array( Array &&inRHS, const Allocator &inAllocator ) noexcept : // TODO: type_identity_t? noexcept?? move iterator assignment???
			Allocator( inAllocator )
		{
			if ( get_allocator() == inRHS.get_allocator() )
			{
				// Allocators match: safe to steal the buffer
				mElements = inRHS.mElements;
				mSize = inRHS.mSize;
				mCapacity = inRHS.mCapacity;

				inRHS.mElements = nullptr;
				inRHS.mSize = 0;
				inRHS.mCapacity = 0;
			}
			else
			{
				// Allocators differ: must move element by element
				assign( std::make_move_iterator( inRHS.begin() ), std::make_move_iterator( inRHS.end() ) );
			}
		}

		// Destructor
		~Array()
		{
			destroy();
		}


		// Copy assignment
		Array& operator=( const Array &inRHS )
		{
			// Do nothing if same
			if ( this == &inRHS ) return *this;

			if constexpr ( alloc_traits::propagate_on_container_copy_assignment::value )
			{
				if ( get_allocator() != inRHS.get_allocator() )
				{
					// Must free with the old allocator before replacing it
					destroy();
				}
				get_allocator() = inRHS.get_allocator();
			}

			assign( inRHS.begin(), inRHS.end() );
			return *this;
		}

		// Move assignment
		Array& operator=( Array &&inRHS ) noexcept
		{
			// Do nothing if same
			if ( this == &inRHS ) return *this;

			if constexpr ( alloc_traits::propagate_on_container_move_assignment::value )
			{
				// Propagation enabled: free our buffer, steal theirs, take their allocator
				destroy();
				get_allocator()     = std::move( inRHS.get_allocator() );
				mElements           = inRHS.mElements;
				mSize               = inRHS.mSize;
				mCapacity           = inRHS.mCapacity;
				inRHS.mElements     = nullptr;
				inRHS.mSize         = 0;
				inRHS.mCapacity     = 0;
			}
			else if ( get_allocator() == inRHS.get_allocator() )
			{
				// Propagation disabled but allocators match: safe to steal
				destroy();
				mElements           = inRHS.mElements;
				mSize               = inRHS.mSize;
				mCapacity           = inRHS.mCapacity;
				inRHS.mElements     = nullptr;
				inRHS.mSize         = 0;
				inRHS.mCapacity     = 0;
			}
			else
			{
				// Propagation disabled and allocators differ: must move element by element
				// Cannot free inRHS's buffer with our allocator, so we leave it intact
				assign( std::make_move_iterator( inRHS.begin() ), std::make_move_iterator( inRHS.end() ) );
			}

			return *this;
		}

		// Initializer list assignment
		Array& operator=( std::initializer_list<T> inList )
		{
			assign( inList );
			return *this;
		}

	private:
		template <typename Iterator>
		void assign_sized( Iterator inFirst, Iterator inLast, size_type inCount )
		{
			// Fast path for trivially copyable types + pointer/contiguous iterators
			if constexpr ( std::is_trivially_copyable_v<T> && std::contiguous_iterator<Iterator> )
			{
				if ( inCount > mCapacity )
				{
					destroy();
					reserve( inCount );
				}
				else
				{
					// TODO: ensure destroy shortcuts for trivally destructable types
				}

				if ( inCount > 0 ) {
					std::memcpy( mElements, std::to_address( inFirst ), inCount * sizeof( T ) ); // TODO: memmove for overlap?
				}

				mSize = inCount;
			}

			// Standard assignment; construct/assign/destroy
			else
			{
				if ( inCount > mCapacity )
				{
					// Destroy, then rebuild
					destroy();
					reserve( inCount );
					for ( ; inFirst != inLast; ++inFirst ) {
						alloc_traits::construct( get_allocator(), mElements + mSize++, *inFirst );
					}
				}
				else
				{
					// Buffer is large enough, assign over existing elements, construct into uninitialised capactiy, destruct any leftovers

					const size_type overlap = std::min( inCount, mSize );

					size_type i = 0;
					for ( ; i < overlap; ++i, ++inFirst ) {
						mElements[i] = *inFirst;
					}

					for ( ; inFirst != inLast; ++inFirst ) {
						alloc_traits::construct( get_allocator(), mElements + mSize++, *inFirst );
					}

					if (inCount < mSize)
					{
						destruct( inCount, mSize );
						mSize = inCount;
					}
				}
			}
		}

	public:

		// Fill assign
		void assign( size_type inCount, const T &inValue )
		{
			// Ensure no self value assignment!
			assert( &inValue < mElements || &inValue >= mElements + mSize );

			// Trivially copyable fast path
			if constexpr ( std::is_trivially_constructible_v<T> )
			{
				if ( inCount > mCapacity )
				{
					destroy();
					reserve( inCount );
				}

				if constexpr (sizeof(T) == 1)
				{
					// memset is valid when T is a single byte type
					std::memset( mElements, static_cast<int>( inValue ), inCount );
				}
				else
				{
					// std::fill handles both the overlap and tail regions since no construction semantics are required for trivial types 
					std::fill( mElements, mElements + inCount, inValue );
				}

				mSize = inCount;
			}

			// Non trivial path
			else
			{
				if (inCount > mCapacity)
				{
					destroy();
					reserve( inCount );

					for ( size_type i = 0; i < inCount; ++i ) {
						alloc_traits::construct( get_allocator(), mElements + i, inValue );
					}

					mSize = inCount;
				}
				else
				{
					const size_type overlap = std::min(inCount, mSize);

					for ( size_type i = 0; i < overlap; ++i ) {
						mElements[i] = inValue;
					}

					for ( size_type i = overlap; i < inCount; ++i ) {
						alloc_traits::construct( get_allocator(), mElements + i, inValue );
					}

					destruct( inCount, mSize );
					mSize = inCount;
				}
			}
		}

		// Iterator range assign
		template <typename Iterator>
		void assign( Iterator inFirst, Iterator inLast )
		{
			if constexpr ( std::is_base_of_v<std::random_access_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category> )
			{
				// Random access
				const size_type count = static_cast<size_type>( std::distance( inFirst, inLast ) );
				assign_sized( inFirst, inLast, count );
			}
			else
			{
				// Input/forward iterators: Clear first and grow as we go
				clear();
				for ( ; inFirst != inLast; ++inFirst ) {
					emplace_back( *inFirst );
				}
			}
		}

		// Initializer list assign
		void assign( std::initializer_list<T> inList )
		{
			assign_sized( inList.begin(), inList.end(), static_cast<size_type>( inList.size() ) );
		}

		//TODO



		// iterators
		iterator begin() noexcept              { return mElements; }
		iterator end() noexcept                { return mElements + mSize; }

		const_iterator begin() const noexcept  { return mElements; }
		const_iterator end() const noexcept    { return mElements + mSize; }

		const_iterator cbegin() const noexcept { return mElements; }
		const_iterator cend() const noexcept   { return mElements + mSize; }

		// reverse iterators
		reverse_iterator rbegin() noexcept              { return reverse_iterator( mElements + mSize ); }
		reverse_iterator rend() noexcept                { return reverse_iterator( mElements ); }

		const_reverse_iterator rbegin() const noexcept  { return const_reverse_iterator( mElements + mSize ); }
		const_reverse_iterator rend() const noexcept    { return const_reverse_iterator( mElements ); }

		const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator( mElements + mSize ); }
		const_reverse_iterator crend() const noexcept   { return const_reverse_iterator( mElements ); }

		// capacity
		bool empty() const noexcept			{ return mSize == 0; }
		size_type size() const noexcept		{ return mSize; }
		size_type max_size() const noexcept	{ return UINT32_MAX; }
		size_type capacity() const noexcept { return mCapacity;  }

		void shrink_to_fit()
		{
			if ( mElements != nullptr )
			{
				if ( mSize == 0 ) {
					deallocate();
				}
				else if ( mCapacity > mSize ) {
					reallocate( mSize );
				}
			}
		}

		void reserve( size_type inNewSize )
		{
			if ( mCapacity < inNewSize ) {
				reallocate( inNewSize );
			}
		}

		void resize( size_type inNewSize )
		{
			destruct( inNewSize, mSize ); // Destruct tail, if any
			reserve( inNewSize ); // Reserve new size, if needed

			if constexpr ( !std::is_trivially_default_constructible_v<T> )
			{
				for ( size_type i = mSize; i < inNewSize; ++i ) {
					alloc_traits::construct( get_allocator(), mElements + i );
				}
			}
			else
			{
				// TODO: should we zero init? Or is blank init okay?
			}

			mSize = inNewSize;
		}

		void resize( size_type inNewSize, const T& inValue )
		{
			// Guard against passing a reference to one of our own elements
			assert( &inValue < mElements || &inValue >= mElements + mSize && "Cannot pass an element of the array to resize()" );

			if ( inNewSize < mSize )
			{
				destruct( inNewSize, mSize );
				mSize = inNewSize;
			}
			else if ( inNewSize > mSize )
			{
				reserve( inNewSize );

				if constexpr ( std::is_trivially_copyable_v<T> && sizeof( T ) == 1 )
				{
					// Single byte trivial type — memset is valid
					std::memset( mElements + mSize, static_cast<int>( inValue ), inNewSize - mSize );
				}
				else
				{
					for ( size_type i = mSize; i < inNewSize; ++i ) {
						alloc_traits::construct( get_allocator(), mElements + i, inValue );
					}
				}

				mSize = inNewSize;
			}
		}

		void clear() noexcept
		{
			destruct(0, mSize);
			mSize = 0;
		}

		//TODO

		// element access
		reference operator[]( size_type n )				{ assert( n < mSize ); return mElements[n]; }
		const_reference operator[]( size_type n ) const { assert( n < mSize ); return mElements[n]; }

		reference at( size_type n )						{ assert( n < mSize ); return mElements[n]; }
		const_reference at( size_type n ) const			{ assert( n < mSize ); return mElements[n]; }

		reference front()								{ assert( mSize > 0 ); return mElements[0]; }
		const_reference front() const					{ assert( mSize > 0 ); return mElements[0]; }

		reference back()								{ assert( mSize > 0 ); return mElements[mSize - 1]; }
		const_reference back() const					{ assert( mSize > 0 ); return mElements[mSize - 1]; }

		// data access
		pointer data() noexcept				{ return mElements; }
		const_pointer data() const noexcept { return mElements; }

		// modifiers
		template<typename... Args>
		reference emplace_back( Args&&... args ) {
			grow();
			alloc_traits::construct( get_allocator(), mElements + mSize, std::forward<Args>( args )... );
			++mSize;
			return back();
		}

		void push_back( const T &inValue )
		{
			assert( &inValue < mElements || &inValue >= mElements + mSize && "Cannot pass an element of the array to push_back()" );

			grow();
			alloc_traits::construct( get_allocator(), mElements + mSize, inValue );
			++mSize;
		}

		void push_back( T &&inValue )
		{
			assert( &inValue < mElements || &inValue >= mElements + mSize && "Cannot pass an element of the array to push_back()" );

			grow();
			alloc_traits::construct( get_allocator(), mElements + mSize, std::move( inValue ) );
			++mSize;
		}

		void pop_back() noexcept( std::is_nothrow_destructible_v<T> )
		{
			--mSize;
			alloc_traits::destroy( get_allocator(), mElements + mSize );
		}

		void swap( Array& inRHS ) noexcept
		{
			if ( this == &inRHS )
				return;

			if constexpr ( std::allocator_traits<Allocator>::propagate_on_container_swap::value )
			{
				// Allocator travels with the data - swap everything including allocator
				std::swap( static_cast<Allocator&>( *this ), static_cast<Allocator&>( inRHS ) );
				std::swap( mElements, inRHS.mElements );
				std::swap( mSize, inRHS.mSize );
				std::swap( mCapacity, inRHS.mCapacity );
			}
			else if ( get_allocator() == inRHS.get_allocator() )
			{
				// Allocators don't propagate but are equal - buffers are
				// interchangeable so we can swap the pointers directly
				std::swap( mElements, inRHS.mElements );
				std::swap( mSize, inRHS.mSize );
				std::swap( mCapacity, inRHS.mCapacity );
			}
			else
			{
				// Allocators don't propagate and are unequal - undefined behaviour
				// per the standard. We assert rather than silently doing something
				// unexpected, since any element-wise swap would leave each buffer
				// owned by the wrong allocator.
				assert( false && "swap() called on arrays with unequal non-propagating allocators" );
			}
		}



		// Allocator helpers
		Allocator&						get_allocator()			{ return *this; }
		const Allocator&				get_allocator() const	{ return *this; }

	private:
		pointer		mElements = nullptr;
		size_type	mSize     = 0;
		size_type	mCapacity = 0;
	};

	template <typename T, typename Allocator>
	inline void swap( Array<T, Allocator> &inLHS, Array<T, Allocator> &inRHS ) noexcept
	{
		inLHS.swap( inRHS );
	}

	// Size conformance, this is the whole point of our smaller array
	static_assert( sizeof( Array<int> ) == 16, "Cyclone::Util::Array exceeds 16 bytes!" );
	static_assert( alignof( Array<int> ) == alignof( void* ), "Cyclone::Util::Array must be pointer-aligned!" );
	static_assert( std::is_standard_layout_v<Array<int>>, "ShortArray must be standard layout" );

	// Sanity checks
	static_assert( std::is_same_v<Array<int>::size_type, uint32_t> );
	static_assert( std::is_same_v<Array<int>::difference_type, std::ptrdiff_t> );
	static_assert( std::is_same_v<Array<int>::value_type, int> );

	// Iterator concept conformance
	static_assert( std::contiguous_iterator<Array<int>::iterator> );
	static_assert( std::contiguous_iterator<Array<int>::const_iterator> );
	static_assert( std::random_access_iterator<Array<int>::reverse_iterator> );
	static_assert( std::random_access_iterator<Array<int>::const_reverse_iterator> );

	// Iterator traits conformance
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::iterator>::value_type, int> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::iterator>::difference_type, std::ptrdiff_t> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::iterator>::iterator_concept, std::contiguous_iterator_tag> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::iterator>::iterator_category, std::random_access_iterator_tag> );

	// Iterator traits conformance (const)
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::const_iterator>::value_type, int> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::const_iterator>::difference_type, std::ptrdiff_t> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::const_iterator>::iterator_concept, std::contiguous_iterator_tag> );
	static_assert( std::is_same_v<std::iterator_traits<Array<int>::const_iterator>::iterator_category, std::random_access_iterator_tag> );

	// Noexcept guarantees
	static_assert( std::is_nothrow_default_constructible_v<Array<int>> );
	static_assert( std::is_nothrow_move_constructible_v<Array<int>> );
	static_assert( std::is_nothrow_move_assignable_v<Array<int>> );
	static_assert( std::is_nothrow_swappable_v<Array<int>> );
	static_assert( std::is_nothrow_destructible_v<Array<int>> );

	// Copy/move
	static_assert( std::is_copy_constructible_v<Array<int>> );
	static_assert( std::is_copy_assignable_v<Array<int>> );

	// Allocator
	static_assert( std::is_same_v<Array<int>::allocator_type, std::allocator<int>> );
	static_assert( std::is_same_v<std::allocator_traits<Array<int>::allocator_type>::value_type, int> );

	// Range concept conformance
	static_assert( std::ranges::contiguous_range<Array<int>> );
	static_assert( std::ranges::sized_range<Array<int>> );

	//
	// BONUS
	//

	// std::sort, std::nth_element, std::partial_sort etc.
	static_assert( std::sortable<Array<int>::iterator> );

	// std::binary_search, std::lower_bound, std::upper_bound, std::equal_range
	static_assert( std::sortable<Array<int>::iterator, std::ranges::less> );

	// std::merge, std::inplace_merge, std::includes, std::set_union etc.
	static_assert( std::mergeable<Array<int>::iterator, Array<int>::iterator, Array<int>::iterator> );

	// std::copy, std::move, std::transform etc.
	static_assert( std::indirectly_copyable<Array<int>::const_iterator, Array<int>::iterator> );
	static_assert( std::indirectly_movable<Array<int>::iterator, Array<int>::iterator> );

	// std::swap_ranges, std::rotate etc.
	static_assert( std::indirectly_swappable<Array<int>::iterator, Array<int>::iterator> );

	// std::equal, std::mismatch, std::lexicographical_compare etc.
	static_assert( std::indirectly_comparable<Array<int>::iterator, Array<int>::iterator, std::ranges::equal_to> );
	static_assert( std::indirectly_comparable<Array<int>::iterator, Array<int>::iterator, std::ranges::less> );

	// std::for_each, std::transform, std::generate etc.
	static_assert( std::indirectly_unary_invocable<std::identity, Array<int>::iterator> );

	// std::permutation, std::next_permutation, std::prev_permutation
	static_assert( std::permutable<Array<int>::iterator> );
}