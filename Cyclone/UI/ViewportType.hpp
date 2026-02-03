namespace Cyclone
{
	namespace UI
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
		struct ViewportTypeTraits<EViewportType::TopXZ>
		{
			static constexpr size_t AxisU = 0;
			static constexpr size_t AxisV = 2;
		};

		template<>
		struct ViewportTypeTraits<EViewportType::FrontXY>
		{
			static constexpr size_t AxisU = 0;
			static constexpr size_t AxisV = 1;
		};

		template<>
		struct ViewportTypeTraits<EViewportType::SideYZ>
		{
			static constexpr size_t AxisU = 2;
			static constexpr size_t AxisV = 1;
		};
	}
}