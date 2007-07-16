#include "callback.h"

namespace ythread {

template <class CLASS>
class CallbackImpl : public Callback {
 public:
  CallbackImpl(CLASS* object, void(CLASS::*func)())
    : object_(object), func_(func) { }
  virtual ~CallbackImpl() { }

  virtual void Execute() {
    (object_->*func_)();
  }

 private:
  CLASS* object_;
  void(CLASS::*func_)();
};

template <class CLASS>
Callback* NewCallback(CLASS* object, void(CLASS::*func)()) {
  return new CallbackImpl<CLASS>(object, func);
}

template <class CLASS, class ARG>
class CallbackArg1Impl : public Callback {
 public:
  CallbackArg1Impl(CLASS* object, void(CLASS::*func)(ARG), ARG arg)
    : object_(object), func_(func), arg_(arg) { }
  virtual ~CallbackArg1Impl() { }

  virtual void Execute() {
    (object_->*func_)(arg_);
  }

 private:
  CLASS* object_;
  void(CLASS::*func_)(ARG);
  ARG arg_;
};

template <class CLASS, class ARG>
Callback* NewCallback(CLASS* object, void(CLASS::*func)(ARG), ARG arg) {
  return new CallbackArg1Impl<CLASS, ARG>(object, func, arg);
}

template <class CLASS, class ARG>
class Callback1Impl : public Callback1<ARG> {
 public:
  Callback1Impl(CLASS* object, void(CLASS::*func)(ARG))
    : object_(object), func_(func) { }
  virtual ~Callback1Impl() { }

  virtual void Execute(ARG arg) {
    (object_->*func_)(arg);
  }

 private:
  CLASS* object_;
  void(CLASS::*func_)(ARG arg);
};

template <class CLASS, class ARG>
Callback1<ARG>* NewCallback(CLASS* object, void(CLASS::*func)(ARG)) {
  return new Callback1Impl<CLASS, ARG>(object, func);
}

}  // namespace ythread
