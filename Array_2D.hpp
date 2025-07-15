//
// Created by maciej on 12.07.25.
//

#ifndef ARRAY_2D_HPP
#define ARRAY_2D_HPP
#include <algorithm>
#include <iostream>
#include <ostream>
#include <vector>

template <typename Type>
class Array_2D {
    public:
        Array_2D(int width, int length, Type initVal = Type()) :
            width_(width), length_(length), data_(width * length, initVal) {};
        Type& operator()(int x, int y)
        {
            return data_[index(x, y)];
        }

        const Type& operator()(int x, int y) const
        {
            if (x < 0 || x >= width_ || y < 0 || y >= length_)
                throw std::out_of_range("Array_2D::operator()");
            return data_[index(x, y)];
        }

        [[nodiscard]] int width() const
        {
            return width_;
        }
        [[nodiscard]] int length() const
        {
            return length_;
        }
        Type* data()
        {
            return data_.data();
        }
        const Type* data() const
        {
            return data_.data();
        }

        Type* beginRow(int y)
        {
            if (y >= 0 && y < length_)
            {
                return data_.data() + y * width_;
            }
            throw std::out_of_range("Array_2D::beginRow");
        }

        Type* endRow(int y)
        {
            if (y >= 0 && y < length_)
            {
                return data_.data() + (y + 1) * width_; // zgodnie z STL zwraca indeks ZA ostatnim elem wiersza
            }
            throw std::out_of_range("Array_2D::endRow");
        }

        const Type* beginRow(int y) const
        {
            if (y >= 0 && y < length_)
            {
                return data_.data() + y * width_;
            }
            throw std::out_of_range("Array_2D::beginRow");
        }
        const Type* endRow(int y) const
        {
            if (y >= 0 && y < length_)
            {
                return data_.data() + (y + 1) * width_;
            }
            throw std::out_of_range("Array_2D::endRow");
        }

        std::vector<Type> getRow(int y)
        {
            if (y < 0 || y >= length_ || beginRow(y) == endRow(y))
            {
                throw std::out_of_range("Array_2D::getRow");
            }

            std::vector<Type> row(width_);
            std::copy(beginRow(y), endRow(y), row.begin());
            return row;
        }

        std::vector<Type> getCol(int x)
        {
            if (x < 0 || x >= width_)
            {
                throw std::out_of_range("Array_2D::getCol");
            }
                std::vector<Type> col(length_);
                for (size_t i = 0; i < length_; i++)
                {
                    size_t colIndex = i * width_ + x;
                    col[i] = data_[colIndex];
                }

                return col;
        }

        Type* begin()
        {
            return data_.data();
        }
        Type* end()
        {
            return data_.data() + width_ * length_;
        }
        const Type* begin() const
        {
            return data_.data();
        }
        const Type* end() const
        {
            return data_.data() + width_ * length_;
        }

        void fill(Type val)
        {
            std::fill(data_.begin(), data_.end(), val);
        }

        void reset()
        {
            fill(Type());
        }

        void assign(int x, int y, const Type& val)
        {
            data_[index(x, y)] = val;
        }

        auto minVal() const
        {
            return std::min_element(data_.begin(), data_.end());
        }

        auto maxVal() const
        {
            return std::max_element(data_.begin(), data_.end());
        }

        void normalize()
        {
            Type maxVal = *maxVal();
            Type minVal = *minVal();

            if (maxVal - minVal != Type())
            {
                std::cout << "Normalizing..." << std::endl;
                for (Type& val : data_)
                {
                    val = (val - minVal) / (maxVal - minVal);
                }
            } else
            {
                std::fill(data_.begin(), data_.end(), Type());
                std::cerr << "maxVal, minVal equal. Normalize to zero." << std::endl;
            }
        }

    private:
        int width_, length_;
        std::vector<Type> data_;
        [[nodiscard]] int index(int x, int y) const
        {
            return x + y * width_;
        };
};



#endif //ARRAY_2D_HPP
