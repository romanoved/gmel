# gmel - gnu make extension library
Based on gnu make [Loaded Object API](http://www.gnu.org/software/make/manual/make.html#Extending-make)  
Required make >= 4.0.
Currently tested only under linux.

The most interesting part of gmel is the embedded python interpreter.
After initialization, execution of an arbitrary python code in the context of the __main__ module is available.
See py_eval and py_call for details.

### features of gmel (`include gmel`)
---
#### $> (automatic variable) - same as $(lastword $^)
#### GMEL_USE_PY2 - if defined, gmel use python2 instead of python3 engine on `include gmel`
---
#### py_eval
Arguments: 1. python_code  
Returns: None  
Action: evaluates given python_code in embedded interpreter.  
Example:
```make
define py_cmd :=
import os
def test(a, b):
    return os.path.normpath(os.path.join(a, b))
endef
$(py_eval $(py_cmd))
```
---
#### py_call
Arguments: 1. callable_name 2+ [optional arguments]  
Returns: result of `gmel_pycall_wrapper(callable_name(*arguments))`  
Action: try to call python callable_name in context of main module  
Details: gmel python engine formats arguments as unicode for python3 and as bytes for python2.
As result of function call it expects None or bytes.
For convenience, there is an automatically applied built-in gmel_pycall_wrapper: str/bytes -> bytes, int -> str(int), and so on.
See gmel_py2/gmel_py3 in source code for concrete rules.  
Example:
```make
$(py_call test,/home/test,../user) # returns /home/user
```
---
#### bind
Arguments: 1. target_name 2. bind_name 3+ [optional command arguments]  
Returns: None  
Action: creates function bind. Optional command arguments adds as first arguments of binded function.  
Example:
```make
_func = $(if $(3),$(1) $(2),$(2))
$(bind func,_func,ARG1)
# nowtime it is possible to call func without $(call)
$(func 10, 20) # returns ARG1 10
```
---
#### bind_r
Arguments: 1. bind_name 2. target_name 3+ [optional command arguments]  
Returns: None  
Action: like bind, except of order of two first arguments.  
Example:
```make
$(bind popen_bind,bind_r,popen)
# with bind_r
$(popen_bind python,/usr/bin/python,-c)
# the same without bind_r:
$(bind python,popen,/usr/bin/python,-c)
```
---
#### popen
Arguments: 1. cmd_bin 2+ [optional command arguments]  
Returns: result of cmd  
Action: call $(error) in case of non-zero exit-code  
Example:
```make
$(popen seq,-w,9,10) # returns 09 10
```
---
#### sshell
Arguments: 1. command  
Returns: result of /bin/bash -e -o pipefail -c command  
Action: call $(error) in case of non-zero exit-code  
Todo: use SHELL/.SHELLFLAGS instead of hard-coded /bin/bash.  
Example:
```make
$(sshell echo 1; false) # fails
```
---
#### strptime
Arguments: 1. date_format 2. date_str  
Returns: unix time (timestamp) in seconds  
Example:
```make
$(strptime %Y-%m-%d,2014-01-11) # returns 1389384000
```
---
#### strftime
Arguments: 1. date_format 2. unix_timestamp  
Returns: date_str  
Example:
```make
$(strftime %Y%m%d,1389384000) # returns 20140111
```
---
#### strptime
Arguments: 1. new_date_format 2. old_date_format 3. date_str  
Returns: date_str in new format  
Example:
```make
$(strfptime %Y%m%d,%Y-%m-%d %H,2014-01-11 23) # returns 20140111
```
---
