#pragma once

#include <iostream>
#include <fstream>

typedef enum { NORMAL, UNIFORM } Distribution;

template <typename T>
class Vector{

	private:
		uint32_t size;
		T* data;
		
	public:
		Vector(uint32_t size);
		Vector(uint32_t size, Distribution distr, T a, T b);
		Vector(const Vector<T>& v);
		Vector(std::ifstream& input, uint32_t size);
		~Vector();

		uint32_t len() const;
		void normal(T mean, T std);
		void uniform(T lower, T upper);

		T& operator[](uint32_t index)const;
		Vector& operator-() const;

		Vector operator+(const Vector& vector) const;
		template <typename U>
		T operator*(const Vector<U>& vector) const;

		Vector& operator+=(const T& scalar);
		Vector& operator*=(const T& scalar);
		Vector& operator/=(const T& scalar);
};

#include "../modules/Vector.cpp"