#!/usr/bin/make -f

$(info MAKE_VERSION=$(MAKE_VERSION))
$(info )


# got from gmsl:
true := \#t
false :=

seq = $(if $(subst x$1,,x$2)$(subst x$2,,x$1),$(false),$(true))

int_inc = $(strip $1 x)
int_decode = $(words $1)

passed :=
failed :=


start_test = $(if $0,$(shell echo -n "Testing '$1': " >&2))$(eval current_test := OK)
stop_test  = $(if $0,$(shell echo " $(current_test)" >&2))
test_pass = .$(eval passed := $(call int_inc,$(passed)))
test_fail = X$(eval failed := $(call int_inc,$(failed)))$(eval current_test := ERROR '$1' != '$2')
test_assert = $(if $0,$(if $(filter undefined,$(origin 2)),$(eval 2 :=))$(shell echo -n $(if $(call seq,$1,$2),$(call test_pass,$1,$2),$(call test_fail,$1,$2)) >&2))

.PHONY: all
all:
	@echo
	@echo Test Summary
	@echo ------------
	@echo "$(call int_decode,$(passed)) tests passed; $(call int_decode,$(failed)) tests failed"
	@$(if $(failed),$(error tests failed))

include gmel
include gmel_py2

$(bind popen_bind,bind_r,popen)

$(popen_bind python_sh,python,-c)
$(popen_bind sequence,seq)
$(popen_bind wsequence,seq,-w)

$(call start_test,gmel_py2)
define py_cmd :=
import sys
import os
def test(a, b):
    return os.path.normpath(os.path.join(a, b))
endef
$(py2_eval $(py_cmd))
define py_cmd :=
def test2(a, b):
    return test(a, b) + '/', 1
endef
$(py2_eval $(py_cmd))
$(call test_assert,$(py2_call test2,/home/test,../user),/home/user/ 1)
$(call stop_test)

$(call start_test,strptime)
$(call test_assert,$(strptime %Y-%m-%d,2014-01-11),1389384000)
$(call stop_test)

$(call start_test,strftime)
$(call test_assert,$(strftime %Y-%m-%d %H,1389384000),2014-01-11 00)
$(call test_assert,$(strftime %Y%m%d,$(strptime %Y-%m-%d,2014-01-11)),20140111)
$(call stop_test)

$(call start_test,strfptime)
$(call test_assert,$(strfptime %Y%m%d,%Y-%m-%d %H,2014-01-11 23),20140111)
$(call stop_test)

$(call start_test,popen_sh)
__popen_result := [0, 1, 2, 3, 4]
$(call test_assert,$(popen python,-c,print(range(5))),$(__popen_result))
$(call stop_test)

$(call start_test,python)
define __python__src :=
import sys
for i in xrange(3):
    print(i,1)
endef
$(call test_assert,$(python_sh $(__python__src)),(0, 1) (1, 1) (2, 1))
$(call stop_test)

$(call start_test,sequence/wsequence)
$(call test_assert,$(sequence 99,100),99 100)
$(call test_assert,$(wsequence 9,10),09 10)
$(call stop_test)

$(call start_test,bind regular functions)
_func = $(if $(2),$(1),$(2))
$(bind func,_func)
$(call test_assert,$(func 10,20),10)
$(call stop_test)

$(call start_test,shell/sshell)
SHELL := /bin/bash -e -o pipefail
__sshell_cmd := echo ok; true | false | true; echo error
$(call test_assert,$(shell $(__sshell_cmd)),ok)
__sshell_submake := include gmel\n$$(info $$(sshell $(__sshell_cmd)))
__sshell_depth := $(if $(call seq,0,$(MAKELEVEL)),,[$(MAKELEVEL)])
__sshell_result := make$(__sshell_depth): *** popen: ['/bin/bash', '-e', '-o', 'pipefail', '-c', 'echo ok; true | false | true; echo error'] exit with code 1.  Stop.
$(call test_assert,$(shell make --no-print-directory --eval $$'$(__sshell_submake)' 2>&1),$(__sshell_result))
$(call stop_test)
