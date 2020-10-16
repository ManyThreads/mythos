#ifndef CHAIN_HH
#define CHAIN_HH

template <typename T>
class Chain{
public:
  Chain();
  ~Chain() = default;

  T getValue();

  Chain<T>* getNext();
  void setNext(Chain<T>* next);

private:
  Chain<T>* next;
  T value;
};

template <typename T>
Chain<T>::Chain(){
  this->next = nullptr;
}

template <typename T>
T Chain<T>::getValue(){
  return this->value();
}

template <typename T>
Chain<T>* Chain<T>::getNext(){
  return this->next;
}

template <typename T>
void Chain<T>::setNext(Chain<T>* next){
  this->next = next;
}

#endif // CHAIN_HH
