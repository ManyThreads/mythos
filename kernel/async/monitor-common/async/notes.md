    template<class... T>
    class IResult
    {
    public:
      virtual ~IResult() {}
      virtual void response(Tasklet*, T const&... v) = 0;
      virtual void error(Tasklet*, Error e) = 0;
    };

    template<class H, class... T>
    class FuncSink
      : public IResult<T...>
    {
    public:
      typedef void (*SuccessFun)(H, Tasklet*, T const&...);
      typedef void (*ErrorFun)(H, Tasklet*, Error);
      FuncSink(H handle, SuccessFun sfun, ErrorFun efun)
	: handle(handle), sfun(sfun), efun(efun) {}
      virtual ~FuncSink() {}
      virtual void response(Tasklet* t, T const&... v) { sfun(handle, t, v...); }
      virtual void error(Tasklet* t, Error e) { efun(handle, t, e); }
    protected:
      H handle;
      SuccessFun sfun;
      ErrorFun efun;
    };

    template<class O, class T, void(O::*SM)(Tasklet*, T), void(O::*EM)(Tasklet*, Error)>
    class MSink
      : public IResult<T>
    {
    public:
      MSink(O* obj) : obj(obj) {}
      virtual ~MSink() {}
      virtual void response(Tasklet* t, T const& v) { (obj->*SM)(t, v); }
      virtual void error(Tasklet* t, Error e) { (obj->*EM)(t, e); }
    protected:
      T* obj;
    };

    template<class O, class H, class T, void(O::*SM)(H, Tasklet*, T), void(O::*EM)(H, Tasklet*, Error)>
    class MHSink
      : public IResult<T>
    {
    public:
      MHSink(O* obj, H handle) : obj(obj), handle(handle) {}
      virtual ~MHSink() {}
      virtual void response(Tasklet* t, T const& v) { (obj->*SM)(handle, t, v); }
      virtual void error(Tasklet* t, Error e) { (obj->*EM)(handle, t, e); }
    protected:
      T* obj;
      H handle;
    };
