#pragma once

#include <memory>
#include <algorithm>
#include <optional>

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <random>

#include "cysift.h"
#include "cell_utils.h"
#include "cell_flag.h"
#include "cell_graph.h"

using namespace std;

/// @brief Enum class representing the different types a Column can have.
enum class ColumnType {
  INT,
  FLOAT,
  STRING,
  FLAG,
  GRAPH
};

/**
 * @class Column
 * @brief Base class for a column in a cell table, which can store elements of 
 *
 * This class provides an interface for columns of different types (integer, float, or string)
 * to be used in a table. Derived classes should implement the specific behavior for each type.
 */
class Column {
public:
  virtual ~Column() = default;
  
  virtual std::shared_ptr<Column> clone() const = 0;
  
  virtual std::shared_ptr<Column> CopyToFloat() const = 0;    
  
  virtual size_t size() const = 0;
  
  virtual std::string toString() const = 0;
  
  virtual ColumnType GetType() const = 0;
  
  virtual void Log10() = 0;
  
  //virtual float GetNumericElem(size_t i) const = 0;
  
  virtual std::string GetStringElem(size_t i) const = 0;
  
  virtual void SubsetColumn(const std::vector<size_t>& indices) = 0;
  
  virtual void PrintElem(size_t i) const = 0;
  
  virtual void SetPrecision(size_t n) = 0;
  
  virtual float Mean() const = 0;
  virtual float Min() const = 0;
  virtual float Max() const = 0;

  virtual void Order(const std::vector<size_t> indicies) = 0;

  virtual void reserve(size_t n) = 0;

  virtual void resize(size_t n) = 0;

  virtual void Scramble(int seed) = 0;
};

template <typename T>
class NumericColumn : public Column {
  static_assert(std::is_arithmetic<T>::value, "NumericColumn requires a numeric type");
  
 public:
  
  NumericColumn() {
    
    if (std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>) {
      m_type = ColumnType::INT;
    } else if (std::is_same_v<T, float>) {
      m_type = ColumnType::FLOAT;
    } else {
      throw std::runtime_error("NumericColumn constructor: arg is neither an int nor a float");
    }
    
  }

  // copy from a normal vector to a Numeric Column
  NumericColumn(const std::vector<T>& vec) {
    
    if (std::is_same_v<T, cy_uint>) {
      m_type = ColumnType::INT;
    } else if (std::is_same_v<T, float>) {
      m_type = ColumnType::FLOAT;
    } else {
      throw std::runtime_error("NumericColumn constructor: arg is neither an int nor a float");      
    }
    
    m_vec = vec;
  }
  
  NumericColumn(const T& initial_elem) {
    
    if (std::is_same_v<T, cy_uint>) {
      m_type = ColumnType::INT;
    } else if (std::is_same_v<T, float>) {
      m_type = ColumnType::FLOAT;
    } else {
      throw std::runtime_error("NumericColumn constructor: arg is neither an int nor a float");      
    }
    
    m_vec.push_back(initial_elem);
  }

  std::shared_ptr<Column> clone() const override {
    return std::make_shared<NumericColumn<T>>(*this);
  }

  T GetNumericElem(size_t i) const {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return m_vec.at(i);
  }

  void Scramble(int seed) override {
    std::default_random_engine rng(seed);
    std::shuffle(m_vec.begin(), m_vec.end(), rng);
  }
  
  std::string GetStringElem(size_t i) const override {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return std::to_string(m_vec.at(i));
  }

  void SetNumericElem(T val, size_t i) {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    m_vec[i] = val;
  }
  
  std::shared_ptr<Column> CopyToFloat() const override {
    
    auto currentColumn = static_cast<const NumericColumn<T>*>(this);
    std::shared_ptr<NumericColumn<float>> fcol = std::make_shared<NumericColumn<float>>(currentColumn->m_vec.size());

    for (size_t i = 0; i < currentColumn->m_vec.size(); ++i) {
      fcol->SetValueAt(i, static_cast<float>(currentColumn->m_vec[i]));
    }

    return fcol;
  }

  void SetValueAt(size_t index, const T& value) {
    if (index > this->size()) {
      throw std::runtime_error("i is out of bonds on SetElem in Column");
    }
    m_vec[index] = value;
  }

  void PushElem(const T& elem) {
    m_vec.push_back(elem);
  }

  float Mean() const override {

    double nr = this->size();
    if (this->size() == 0) {
      throw std::runtime_error("Cannot compute the mean of an empty column.");
    }

    double sum = std::accumulate(m_vec.begin(), m_vec.end(), 0.0);
    double mean = sum / m_vec.size();
    
    return sum / nr;
  }
  
  float Min() const override {
    
    if (this->size() == 0) {
      throw std::runtime_error("Cannot compute the min of an empty column.");
    }
    return static_cast<float>(*std::min_element(m_vec.begin(), m_vec.end()));
  }
  
  float Max() const override {
    if (this->size() == 0) {
      throw std::runtime_error("Cannot compute the max of an empty column.");
    }
    
    return static_cast<float>(*std::max_element(m_vec.begin(), m_vec.end()));
  }
  
  size_t size() const override {
    return m_vec.size();
  }
  
  std::string toString() const override {
    const size_t print_lim = 3;
        std::stringstream ss;
        ss << "Numeric<" << typeid(T).name() << ">: Size: " <<
	  m_vec.size() << " [";
        for (size_t i = 0; i < std::min(size(), print_lim); i++) {
	  if (i > 0) ss << ", ";
	  ss << m_vec[i];
        }
        if (size() > print_lim) ss << ", ...";
        ss << "]";
	ss << " Min: " << this->Min() << " - Max: " << this->Max() << " - Mean: " << this->Mean();
        return ss.str();
    }

    void Log10() override {
      for (auto& elem : m_vec) {
	if (elem <= 0) {
	  throw std::invalid_argument("Log10 encountered a non-positive value");
	}
	elem = std::log10(elem);
      }
    }

    void SubsetColumn(const std::vector<size_t>& indices) override {
      std::vector<T> new_vec;
      new_vec.reserve(indices.size());
      for (const auto& index : indices) {
	if (index < 0 || index >= m_vec.size()) {
	  throw std::out_of_range("Index out of range");
	}
	new_vec.push_back(m_vec[index]);
      }
      m_vec = std::move(new_vec);
    }
    
    // Add this method to NumericColumn, StringColumn
    ColumnType GetType() const override {
      return m_type;
    }

    void PrintElem(size_t i) const override {
      
      if (i >= m_vec.size()) {
	throw std::runtime_error("Out of bounds in PrintElem");
	return;
      }
      
      switch (m_type) {
      case ColumnType::FLOAT:
        if (m_precision.has_value()) {
          std::cout << std::fixed << std::setprecision(m_precision.value()) <<
	    m_vec.at(i);
        } else {
          std::cout << m_vec.at(i);
        }
        break;
      case ColumnType::INT:
        std::cout << m_vec.at(i);
        break;
      default:
	throw std::runtime_error("Unexpected NumericColumn type");	
        break;
      }
    }
    
  void SetPrecision(size_t n) override {
    m_precision = n;
  }
  
  const std::vector<T>& getData() const {
    return m_vec;
  }

  // Overloading at() for const access
  const T& at(std::size_t index) const {
    return m_vec.at(index);
  }
  
  // Overloading operator[] for non-const access
  T& operator[](std::size_t index) {
    return m_vec[index];
  }
  
  // Const overload for operator[]
  const T& operator[](std::size_t index) const {
    return m_vec[index];
  }
  
  // Return iterator to the beginning
  typename std::vector<T>::iterator begin() {
    return m_vec.begin();
  }
  
  // Return iterator to the end
  typename std::vector<T>::iterator end() {
    return m_vec.end();
  }
  
  // For const-correctness, also provide const versions of begin() and end()
  typename std::vector<T>::const_iterator begin() const {
    return m_vec.begin();
  }
  
  typename std::vector<T>::const_iterator end() const {
    return m_vec.end();
  }
  
  
  std::vector<T> copyData() const {
    return m_vec;
  }

  void reserve(size_t n) override {
    m_vec.clear();
    m_vec.reserve(n);
  }

  void resize(size_t n) override {
    m_vec.resize(n);
  }

  void Order(const std::vector<size_t> indicies) override {
    std::vector<T> tmp_vec(this->size());
    for (size_t i = 0; i < indicies.size(); i++) {
      tmp_vec[i] = m_vec.at(indicies.at(i));
    }
    m_vec = tmp_vec;
  }
  
protected:
    
    std::vector<T> m_vec;
    
    ColumnType m_type;

    std::optional<size_t> m_precision;
};

/*

class StringColumn : public Column {
public:
  StringColumn() = default;
  
  StringColumn(const std::string& initial_elem) {
    m_vec.push_back(initial_elem);
  }
  
  std::string GetStringElem(size_t i) const override {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return m_vec.at(i);
  }

  void Scramble(int seed) override {
    std::default_random_engine rng(seed);
    std::shuffle(m_vec.begin(), m_vec.end(), rng);
  }

  std::shared_ptr<Column> clone() const override {
    return std::make_shared<StringColumn>(*this);
  }
  
  ColumnType GetType() const override {
    return ColumnType::STRING; 
  }

  void SetValueAt(size_t index, const std::string& value) {
    if (index > this->size()) {
      throw std::runtime_error("i is out of bonds on SetElem in Column");
    }
    m_vec[index] = value;
  }
  
 void PushElem(const std::string& elem) {
    m_vec.emplace_back(elem);
  }
  
  size_t size() const override {
    return m_vec.size();
  }
  
  std::string toString() const override {
        std::stringstream ss;
        ss << "StringColumn<string>: [";
        for (size_t i = 0; i < std::min(size(), (size_t)3); i++) {
	  if (i > 0) ss << ", ";
	  ss << m_vec[i];
        }
        if (size() > 3) ss << ", ...";
        ss << "]";
        return ss.str();
  }
  
  // Do nothing for StringColumn (or have dummies)
  void Log10() override {}
  float Mean() const override { return 0;   }
  float Min() const override { return 0;   }
  float Max() const override { return 0;   }
  void SetPrecision(size_t n) override {}
  
  std::shared_ptr<Column> CopyToFloat() const override {
    std::shared_ptr<NumericColumn<float>> fcol =
      std::make_shared<NumericColumn<float>>(1);
    return fcol;
  }
  
  void SubsetColumn(const std::vector<size_t>& indices) override {
    std::vector<std::string> new_vec;
    new_vec.reserve(indices.size());
    for (auto i : indices) {
      if (i >= 0 && i < m_vec.size()) {
	new_vec.push_back(m_vec[i]);
      } else {
	throw std::out_of_range("SubsetColumn: index out of range");
      }
    }
    m_vec = std::move(new_vec);
  }
  
  void PrintElem(size_t i) const override {
    
    if (i < m_vec.size()) {
      std::cout << m_vec.at(i);
    } else {
      throw std::runtime_error("Out of bounds in PrintElem");
    }
  }

  void reserve(size_t n) override {
    m_vec.clear();
    m_vec.reserve(n);
  }

  void resize(size_t n) override {
    m_vec.resize(n);
  }

  void Order(const std::vector<size_t> indicies) override {
    std::vector<std::string> tmp_vec(this->size());
    for (size_t i = 0; i < indicies.size(); i++) {
      tmp_vec[i] = m_vec.at(indicies.at(i));
    }
    m_vec = tmp_vec;
  }

  
 private:
  std::vector<std::string> m_vec;
};


class GraphColumn : public Column {
  
 public:
 
  GraphColumn() = default;

  GraphColumn(const CellNode& initial_elem) {
    m_vec.push_back(initial_elem);
  }

  void Scramble(int seed) override {
    std::default_random_engine rng(seed);
    std::shuffle(m_vec.begin(), m_vec.end(), rng);
  }

  
  std::string GetStringElem(size_t i) const override {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return m_vec.at(i).toString();
  }

  
  

  
  std::shared_ptr<Column> clone() const override {
    return std::make_shared<GraphColumn>(*this);
  }
  
  ColumnType GetType() const override {
    return ColumnType::GRAPH; 
  }
  
  void PushElem(const CellNode& elem) {
    m_vec.emplace_back(elem);
  }

  void SetValueAt(size_t index, const CellNode& value) {
    if (index > this->size()) {
      throw std::runtime_error("i is out of bounds on SetElem in GraphColumn");
    }
    m_vec[index] = value;
  }

  
  size_t size() const override {
    return m_vec.size();
  }

  const CellNode& GetNode(size_t index) const {
    if (index > this->size()) {
      throw std::runtime_error("i is out of bounds on GetNode in GraphColumn");
    }
    return m_vec[index];
  }
  
  std::string toString() const override {
        std::stringstream ss;
        ss << "GraphColumn<CellNode>: [";
        for (size_t i = 0; i < std::min(size(), (size_t)3); i++) {
	  if (i > 0) ss << ", ";
	  ss << m_vec[i];
        }
        if (size() > 3) ss << ", ...";
        ss << "]";
        return ss.str();
  }

  // do nothing for GraphColumn (or have dummies)
  void Log10() override {}
  float Mean() const override { return 0;   }
  float Min() const override { return 0;   }
  float Max() const override { return 0;   }
  void SetPrecision(size_t n) override {}
  std::shared_ptr<Column> CopyToFloat() const override {
    std::shared_ptr<NumericColumn<float>> fcol =
      std::make_shared<NumericColumn<float>>(1);
    return fcol;
  }
  
  void SubsetColumn(const std::vector<size_t>& indices) override {
    std::vector<CellNode> new_vec;
    new_vec.reserve(indices.size());
    for (auto i : indices) {
      if (i >= 0 && i < m_vec.size()) {
	new_vec.push_back(m_vec[i]);
      } else {
	throw std::out_of_range("SubsetColumn: index out of range");
      }
    }
    m_vec = std::move(new_vec);
  }
  
  void PrintElem(size_t i) const override {
    
    if (i < m_vec.size()) {
      std::cout << GetStringElem(i); //m_vec.at(i);
    } else {
      throw std::runtime_error("Out of bounds in PrintElem");
    }
  }

  void reserve(size_t n) override {
    m_vec.clear();
    m_vec.reserve(n);
  }

  void resize(size_t n) override {
    m_vec.resize(n);
  }

  void Order(const std::vector<size_t> indicies) override {
    std::vector<CellNode> tmp_vec(this->size());
    for (size_t i = 0; i < indicies.size(); i++) {
      tmp_vec[i] = m_vec.at(indicies.at(i));
    }
    m_vec = tmp_vec;
  }

  
 private:

  std::vector<CellNode> m_vec;

};
*/
/*
class FlagColumn : public Column {

 public:
 
  FlagColumn() = default;

  FlagColumn(const std::shared_ptr<NumericColumn<cy_uint>> st) {
    this->reserve(st->size());
    for (size_t i = 0; i < st->size(); i++) {
      this->PushElem(CellFlag(st->GetNumericElem(i)));
    }
  }

  void Scramble(int seed) override {
    std::default_random_engine rng(seed);
    std::shuffle(m_vec.begin(), m_vec.end(), rng);
  }
  
  bool TestFlagAndOr(cy_uint logor, cy_uint logand, size_t i) const {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return m_vec.at(i).testAndOr(logor, logand);
  }

  void SetFlagOn(size_t n, size_t i) {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    m_vec[i].setFlagOn(n);
  }
  void SetFlagOff(size_t n, size_t i) {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    m_vec[i].setFlagOff(n);    
  }
  
  
  FlagColumn(const CellFlag& initial_elem) {
    m_vec.push_back(initial_elem);
  }

  std::string GetStringElem(size_t i) const override {
    if (i >= m_vec.size())
      throw std::out_of_range("Index out of range");
    return m_vec.at(i).toString();
  }
  
  std::shared_ptr<Column> clone() const override {
    return std::make_shared<FlagColumn>(*this);
  }
  
  ColumnType GetType() const override {
    return ColumnType::FLAG; 
  }
  
  void PushElem(const CellFlag& elem) {
    m_vec.emplace_back(elem);
  }
  
  size_t size() const override {
    return m_vec.size();
  }
  
  std::string toString() const override {
        std::stringstream ss;
        ss << "Flag: [";
        for (size_t i = 0; i < std::min(size(), (size_t)3); i++) {
	  if (i > 0) ss << ", ";
	  ss << m_vec[i];
        }
        if (size() > 3) ss << ", ...";
        ss << "]";
        return ss.str();
  }

  // do nothing for FlagColumn (or have dummies)
  void Log10() override {}
  float Mean() const override { return 0;   }
  float Min() const override { return 0;   }
  float Max() const override { return 0;   }
  void SetPrecision(size_t n) override {}
  std::shared_ptr<Column> CopyToFloat() const override {
    std::shared_ptr<NumericColumn<float>> fcol =
      std::make_shared<NumericColumn<float>>(1);
    return fcol;
  }
  
  void SubsetColumn(const std::vector<size_t>& indices) override {
    std::vector<CellFlag> new_vec;
    new_vec.reserve(indices.size());
    for (auto i : indices) {
      if (i >= 0 && i < m_vec.size()) {
	new_vec.push_back(m_vec[i]);
      } else {
	throw std::out_of_range("SubsetColumn: index out of range");
      }
    }
    m_vec = std::move(new_vec);
  }
  
  void PrintElem(size_t i) const override {
    
    if (i < m_vec.size()) {
      std::cout << m_vec.at(i);
    } else {
      throw std::runtime_error("Out of bounds in PrintElem");
    }
  }

    void reserve(size_t n) override {
    m_vec.clear();
    m_vec.reserve(n);
  }

  void resize(size_t n) override {
    m_vec.resize(n);
  }

  void Order(const std::vector<size_t> indicies) override {
    std::vector<CellFlag> tmp_vec(this->size());
    for (size_t i = 0; i < indicies.size(); i++) {
      tmp_vec[i] = m_vec.at(indicies.at(i));
    }
    m_vec = tmp_vec;
  }

  
 private:

  std::vector<CellFlag> m_vec;
  
};
*/

// aliases
using IntCol = NumericColumn<cy_uint>;
using IDCol = NumericColumn<uint64_t>;
using FloatCol = NumericColumn<float>;

using IDColPtr    = std::shared_ptr<IDCol>;
//using GraphColPtr = std::shared_ptr<GraphColumn>;
using IntColPtr   = std::shared_ptr<IntCol>;
using FloatColPtr = std::shared_ptr<FloatCol>;
using StringColPtr= std::shared_ptr<StringColumn>;
//using FlagColPtr  = std::shared_ptr<FlagColumn>;
using ColPtr      = std::shared_ptr<Column>;
