#pragma once

namespace Cyclone::UI
{
	enum class EViewportType
	{
		Perspective,
		TopXZ,
		FrontXY,
		SideYZ
	};

	template<EViewportType T>
	struct ViewportTypeTraits;

	template<>
	struct ViewportTypeTraits<EViewportType::Perspective>
	{
		using DrawTag = entt::tag<"draw_perspective"_hs>;
	};

	template<>
	struct ViewportTypeTraits<EViewportType::TopXZ>
	{
		using DrawTag = entt::tag<"draw_top"_hs>;
		static constexpr size_t AxisU = 0;
		static constexpr size_t AxisV = 2;
		static constexpr size_t AxisW = 1;
	};

	template<>
	struct ViewportTypeTraits<EViewportType::FrontXY>
	{
		using DrawTag = entt::tag<"draw_front"_hs>;
		static constexpr size_t AxisU = 0;
		static constexpr size_t AxisV = 1;
		static constexpr size_t AxisW = 2;
	};

	template<>
	struct ViewportTypeTraits<EViewportType::SideYZ>
	{
		using DrawTag = entt::tag<"draw_side"_hs>;
		static constexpr size_t AxisU = 2;
		static constexpr size_t AxisV = 1;
		static constexpr size_t AxisW = 0;
	};
}