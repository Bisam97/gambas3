## To Do


## Work

- IDE: Reliability of Tab 'Unit tests'.
    > When hit F4 with a newly opened project with tabs the tab just flickers but does not show up. After it is opened by hand with the mouse, the next F4 shows the tail of the TAP output, which is right. But if then the tab is resized with the mouse the next F4 shows the output anywhere in the middle and not the tail. The last one cannot reproduced always.

## Done

- Plan selftests
    > gb.test: Every subtest in all tests must plan the own assertion count
- Write Documentation
    * [ ] Test.Main overall doc what is does and how tests are run.
- Event handlers are not test methods
    > Some test modules test object which emit events, but the event handlers are Public Subs which get picked up as test methods by gb.test. What can we do about it?
- TestAllAssertion must test all assertions
- Decide where Skip, SkipAll, Todo reside
    > SkipAll is in Test, Skip in Assert, Todo in Assert. This is inconsistent, We have to decide where they should be. Chris: I am for Test.
- Install gbt3 at Gambas installation
    > gbt3 is compiled and installed by make but there is no symlink gbt3 > gbt3.gambas
- Ensure TAP created according to  spec
    > Tested TAP via gbt3 and python tappy
- gbt3 has to return exit status at the end
- Assert and Test documentation
    > Every public method in Test and Assert has to have a description
- Hide symbols in Test
    > There are symbols in Test which should not be shown to the enduser. These shoult be hidden by a leading underscore:
    * [ ] Subtest
    * [ ] Finish
    * [ ] Reset
- No Bug: Directives always 0
    > Was no bug, was lack of understanding
- TestAssertion: Skip and Todo
    > TestAssertion must contain variables to store Skip and Todo for summary. See TestSummary.
- Todo Directive
    > Todo directive does not exist. See TestSummary. Was wrong. Todo exists.
- Free Assert from non-assertions
    > There are things in Assert which are no assertions, like Print, Finish, Note and so on. These should be methods of Test. For example: Test.Plan, Test.Note, Test.Finish ...
- Test.Main has to complain if testmodule doesn't exist
- Human readability of TAP output
- Subtests don't count the right way
    > If there is only one testmodule called, subtest counts a lot of subtests instead just one
- gb.test doesn't print to console
- Summary as a TAP comment
    > Description in .hidden/summary-example-txt
    * [x] New method Test.ShowTodos
    * [x] New method Test.ShowSkips
- Parser parse subtests
- Skip all
    > Parse `1..0 # SKIP` style TAPs indicating that a whole test was skipped.
- Document Approximate and RelativeApproximate
- Plan
    > If a plan failes it has to be reported in the TAP output as well as in the summary which is not the case at the moment.
