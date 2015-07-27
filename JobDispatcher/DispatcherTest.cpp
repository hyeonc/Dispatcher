#include "stdafx.h"

#include "JobDispatcher.h"

enum TestEnvironment
{
	TEST_OBJECT_COUNT = 10,
	TEST_WORKER_THREAD = 4,
};

class TestObject : public AsyncExecutable
{
public:
	TestObject() : mTestCount(0)
	{}

	void TestFunc0()
	{
		++mTestCount;
	}

	void TestFunc1(int b)
	{
		mTestCount += b;
	}

	void TestFunc2(double a, int b)
	{
		mTestCount += b;

		if (a < 50.0)
			DoAsync(GetSharedFromThis<TestObject>(), &TestObject::TestFunc1, std::move(b));
	}

	void TestFuncForTimer(int b)
	{
		if (rand() % 2 == 0)
			DoAsyncAfter(1000, GetSharedFromThis<TestObject>(), &TestObject::TestFuncForTimer, std::move(-b));
	}

	int GetTestCount() { return mTestCount; }

private:
	int mTestCount;

};

std::vector<std::shared_ptr<TestObject>> gTestObjects;

std::shared_ptr<TestObject> PickRandomTestObject()
{
	return gTestObjects[rand() % TEST_OBJECT_COUNT];
}

class TestWorkerThread : public Runnable
{
public:
	~TestWorkerThread()
	{
		printf("tt dtor\n");
	}

	virtual bool Run()
	{
		/// TEST
		uint32_t after = rand() % 2000;

		if (after > 1000)
		{
			DoAsync(PickRandomTestObject(), &TestObject::TestFunc0);
			DoAsync(PickRandomTestObject(), &TestObject::TestFunc2, double(rand() % 100), 2);
			DoAsync(PickRandomTestObject(), &TestObject::TestFunc1, 1);

			DoAsyncAfter(after, PickRandomTestObject(), &TestObject::TestFuncForTimer, (int)after);
		}

		/// exit condition
		if (PickRandomTestObject()->GetTestCount() > 5000)
		{
			std::cout << "thread " << std::this_thread::get_id() << " end by force\n";
			return false;
		}

		return true;
	}
};



int _tmain(int argc, _TCHAR* argv[])
{
	for (int i = 0; i < TEST_OBJECT_COUNT; ++i)
		gTestObjects.push_back(std::make_shared<TestObject>());

	JobDispatcher<TestWorkerThread> workerService(TEST_WORKER_THREAD);

	workerService.RunWorkerThreads();

	for (auto& t : gTestObjects)
		std::cout << "TestCount: " << t->GetTestCount() << std::endl;

	
	getchar();
	

	return 0;
}
