#include "ThreadGroup.hh"
#include "Tasklet.h"
#include "MonitorHomed.h"
#include "trace.h"

#include <assert.h>
#include <unistd.h>

thread_local size_t threadID;


class AdderService
{
public:
  AdderService(Place* home) : monitor(home) {}
  
  template<class R>
  void add(Tasklet* msg, R* res, int a, int b) {
    trace::recv(msg, this, res, "AdderService::add");
    monitor.exclusive(msg, [=](Tasklet* m){this->addImpl(m,res,a,b);} );
  }

protected:
  MonitorHomed monitor;

  template<class R>
  void addImpl(Tasklet* msg, R* res, int a, int b) {
    trace::begin(msg, this, res, "AdderService::add");
    res->addResult(msg, a+b);
    monitor.release();
  }
};


class TestClient
{
public:
  TestClient(Place* home) : monitor(home), state(0), adder(0) {}
  ~TestClient() {
    IOLOCK(std::cout << Place::id() << ": client " << this << " is being deleted" <<std::endl);
  }
  
  void setAdder(AdderService* adder) { this->adder = adder; }
  
  void run() {
    trace::begin(0, this, "TestClient::run");
    monitor.enter(); // for the request handling
    assert(state==0);
    protothread();
  }

  void doDelete(Tasklet* msg, ExitService* exitService) {
    trace::recv(msg, this, exitService, "TestClient::doDelete");
    monitor.doDelete(msg, [=](Tasklet* m) { delete this; exitService->exitScheduler(); } );
  }
  
protected:
  friend class AdderService;
  void addResult(Tasklet* msg, int sum) {
    trace::recv(msg, this, "TestClient::addResult");
    monitor.exclusive(msg, [=](Tasklet* m){this->addResultImpl(m,sum);} );
  }

  void addResultImpl(Tasklet* msg, int sum) {
    trace::begin(msg, this, "TestClient::addResult");
    this->sum = sum;
    protothread();
  }

  void protothread() {
    switch (state) {
    case 0:
      sum = 0;
      for (iter = 0; iter<2; iter++) {
	state = 1;
	adder->add(&m, this, sum, 1);
	return;
      case 1:
	monitor.release(); // for the response handling
	sleep(1);
      }
    }
    state = 0;
    monitor.release(); // for the request handling
  }
  
protected:
  MonitorHomed monitor;
  size_t state;
  AdderService* adder;
  Tasklet m;
  
  size_t sum;
  size_t iter;
};

struct MyTask
  : public Task
{
  MyTask()
  {
  }
  
  virtual ~MyTask() { }

  virtual void init() {
    places.resize(numThreads());
  }
  
  virtual void run(size_t rank) {
    // init the shared place table
    places[rank] = new Place(rank);
    barrier();
    if (rank == 0) {
      ExitService exitService(&places[0], numThreads());
      // create a test client and bind to own place
      TestClient* tester = new TestClient(places[0]);
      // create an adder and bind to another place
      AdderService adder(places[0]); // 1%numThreads()

      tester->setAdder(&adder);
      tester->run();

      Tasklet delMsg;
      tester->doDelete(&delMsg, &exitService);
    }

    places[rank]->loop();
  }

  std::vector<Place*> places;
};

int main(int argc, char** argv)
{
  MyTask t;
  ThreadGroup gr(2);

  // mapping to logical cores should be driven by policy and scheduling
  //  for (size_t i=0; i<gr.size(); i++) {
  //    gr.setVcore(i, i);
  //  }
  gr.setVcore(0, 0);
  gr.setVcore(1, 4);
  
  gr.start(&t);
  gr.join();
  
  return 0;
}
