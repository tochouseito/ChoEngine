#pragma once

#include <memory>

namespace physics::d3
{
	class Id3World;

	// 3D物理エンジンの形状クラス
	class Id3Shape
	{
	public:
		virtual ~Id3Shape() = default;
	};

	class bulletShape : public Id3Shape
	{
		public:
		bulletShape() = default;
		~bulletShape() override = default;
	private:
		struct Impl;
		std::unique_ptr<Impl> impl; // Bulletの形状データを保持
	};

	class chophysicsShape : public Id3Shape
	{
		public:
		chophysicsShape() = default;
		~chophysicsShape() override = default;
	};
}

