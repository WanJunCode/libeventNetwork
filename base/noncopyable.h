#ifndef WJ_NOCOPYABLE
#define WJ_NOCOPYABLE

class noncopyable
{
public:
  noncopyable(const noncopyable &) = delete;
  void operator=(const noncopyable &) = delete;

protected:
  noncopyable() = default;
  ~noncopyable() = default;
};


#endif // !WJ_NOCOPYABLE