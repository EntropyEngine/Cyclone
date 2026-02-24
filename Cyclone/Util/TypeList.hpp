#pragma once

namespace Cyclone::Util
{
	/// @brief Applies a functor's `.Apply(...)` method over all types in the TypeList with all provided arguments
	/// @tparam TypeList The entt::type_list of types to apply the functor over
	/// @tparam Index The current type index in entt::type_list to call .apply(...) with. For first invocation, ommit template parameter to default to 0
	/// @param functor The functor struct containing `void Apply(...) const`
	/// @param ...args The arguments to provide to `functor.Apply`
	template<typename TypeList, size_t Index = 0>
	void ApplyOverTypeList( const auto &functor, auto&&... args )
	{
		// Invokes functor.Apply(args...) to current Index in type list
		functor.template Apply<entt::type_list_element_t<Index, TypeList>>( std::forward<decltype( args )>( args )... );
		
		// Recursively calls ApplyOverTypeList if more types remain
		if constexpr ( Index + 1 < TypeList::size )
		{
			ApplyOverTypeList<TypeList, Index + 1>( functor, std::forward<decltype( args )>( args )... );
		}
	}
}