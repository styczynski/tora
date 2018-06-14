# MultiTool tora

As a project for Warsaw University course I made an extension to Tora tool interface.<br>
The main purpose of this tool is to allow Tora users to execute statements in multiple connections as this is very useful feature not offered by standard Tora distribution.

## About

### Integration

The tool was integrated with basic "run statement" Tora tool to allow users easily perform common operation on input SQL statements.

### Features of the tool

* The upper menu displays last action (done in the tab) so you can quickly relauch last action
* The collapsible menu on the right side of the last action button has all new multi action there
* On the bottom the pane "Executions" is displayed with current state of execution of all statements

### Running modes

The allowable running modes are:

* **Execute current statement**<br>
  Executes current statement in the focused connection tab (standard Tora behaviour)
  
* **Execute in all opened connections**<br>
  Executes the statement in all of the opened connection tabs.<br>
  The results will be available in the results tab of each connection (Tora standard)
  
* **For each connection execute it's current edited statement**
  Tora executes statements for each opened tab.<br>
  It's equal behaviour to switching to one tab after another and pressing "Execute current statement", but automated.
  The main difference to "Execute in all opened connections" is that the mentioned action executes *one single query* for each connection.
  And this action executes *for each connection the contents of it's editor*
 
* **Execute in choosen opened connections**<br>
  This action shows dialog with list of selectable connections that your statement will be executed in.

## Changes in the code

All changed sources are withing `tora/src/tools` directory.

## Detailed overview

Here are schreenshots from the Multitool:

### Overall view

![Overall view screenshot](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_overall.png)

This is overall view of the new Tora interface that Multitool offers.

### See status of execution

![Status view query is ready](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_status_ok.png)
![Status view query is running](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_status_running.png)

The status view shows execution times of statements in milliseconds.

### Expanadble menu of actions

![Action menu screenshot](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_action_menu.png)

You can select you action then the main button with be changed to the last executed action.

### Select connection to run query in the modal dialog

![Conenction dialog screenshot](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_connection_dialog.png)

The connections can be selected freely with checkboxes.

### See the results in Results tab

![Results tab screenshot](https://raw.githubusercontent.com/styczynski/tora/master/static/tora_multitool_screenshot_result.png)

The results of the query are presented as normally in the standard Tora tab you are familiar with.

*Piotr Styczyński 2018, summer term,  Warsaw University*
