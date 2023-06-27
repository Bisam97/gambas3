# gb.test – the Gambas Unittest

A Gambas component for unittesting and test-driven programming. With this component you can develop software in a test-driven matter (write test first, program functionality afterwards) and you are able to ensure that on refactoring the desired results of your code stays the same.

Tests are output as [Tap](https://testanything.org/) so that they can be displayed, analyzed or viewed with any [Tap consumer](https://testanything.org/consumers.html). As each output includes a summary at the end  with the string "PASSED" or "FAILED" at the last line you can even view the console output to decide whether the test has been successful or not.

## How it works

There is an example in [this simple Gambas project](unittesthelloworld-0.0.8.tar.gz).

### Testmodule

Start by creating a Testmodule, this is a class with any name, but the ending ".test", for example "TestHelloWorld.test". This class contains one or more public Subs, from which each represent a so called Testmethod.

----
    ' Gambas test module file
    ''' Testmodule TestHelloWorld

    '' Testmethod TestHelloWorld
    Public Sub TestHelloWorld()

        Assert.EqualsString("Hello World", Hello.World(), "Strings should be equal")

    End
----

### Module(Function) to test:

To make it work, we need a funktion that will be tested. So we create a function "World" in a module "Hello" in our project:

----

    ' Gambas module file

    ''' Module is named "Hello"

    Public Function World() As String

      Return "Hello World"

    End

----

###  Bringing Test into play

A simple way to execute the Unittest is to create another module, name it "TestMe" or something more interesting and make it a Gambas startclass:

----

    'Module TestMe

    'Is a startclass
    'Starts the Unittest, when F5 was hit

    Public Sub Main()

        Test.Main()

    End

----

If you did all this correctly and now hit &lt;F5&gt;, Gambas will execute the startfunction in module TestMe, which runs through the Testmethods of our Testmodule and presents the test result in the console, for example (this is from unittesthelloworld):

    1..3
    
    Testmethod TestHello:TestFortyTwo
      ok 1 - Test Hello.FortyTwo
      1..1
    ok 1 - TestHello:TestFortyTwo
    
    Testmethod TestHello:TestHelloWorld
      ok 1 - Test equal strings just for fun
      ok 2 - HW strings should be equal
      1..2
    ok 2 - TestHello:TestHelloWorld
    
    Testmethod TestHello:TestRight
      ok 1 - Test Hello.Right says Right
      1..1
    ok 3 - TestHello:TestRight
    
    # Ran: '' 
    #
    # PASSED
    
If a failure occurs it will report FAILED instead of PASSED and will show you the place of the failure. I you want to debug the code you can set a breakpoint inside TestHello.TestRight, hit &lt;F5&gt; again and start debugging.

If you have a lot of tests, and want to let run just one, you can do that like so:

----

    'Module TestMe

    'Is a startclass
    'Starts the Unittest, when F5 was hit

    Public Sub Main()

        Test.Main("TestHello.TestRight")

    End

----

You can combine any number of testmodules and/or testmethods For example like this:

    Test.Main("TestmoduleOne.TestmethodOne, TestmoduleOne.TestmethodTwo, TestmoduleTwo")

In this case only two specific tests in TestmoduleOne and all in TestmoduleTwo will run.

Testmodule and Testmethods can be named the same way as any Gambas Module or Method except that a Testmethod may not be named _Setup(), _Teardown(), _SetupEach() or _TeardownEach().

## Test your project on the console

You also can test your on the console. The command **/usr/bin/gbt3 /path/to/my/project** executes the unittests and prints the result to standard output:

    1..3

    Testmethod TestHello:TestFortyTwo
      ok 1 - Test Hello.FortyTwo
      1..1
    ok 1 - TestHello:TestFortyTwo

    Testmethod TestHello:TestHelloWorld
      ok 1 - Test equal strings just for fun
      ok 2 - HW strings should be equal
      1..2
    ok 2 - TestHello:TestHelloWorld

    Testmethod TestHello:TestRight
      ok 1 - Test Hello.Right says Right
      1..1
    ok 3 - TestHello:TestRight

    # Ran: '' 
    #
    # PASSED

## Test fixture

Sometimes it is neccessary to create a "fixture", a special environment for a test or a couple of tests, and to destroy this environment after the test is done. For example a database connection should be established at the beginning of all tests, some tables for testing should be created and deleted for every single testmethod and the database connection should be closed at the end. This can be done with Setup... and Teardown... functions inside the Testmodule.

### Sub _SetupEach() and Sub _TeardownEach()

You can create methods with these names to create an environment for each testmethod before it is invoked and to destroy it afterwards. If you have five testmethods inside your testmodule these functions will be invoked five times, _SetupEach() before each testmethod, _TeardownEach() after each testmethod. Got it?

### Sub _Setup() and Sub _Teardown()

You can create methods with these names to create an environment for all testmethods inside a testmodule, in the beginning _Setup() is invoked and after all testmethods inside the testmodule are done you can destroy the environment with _Teardown().