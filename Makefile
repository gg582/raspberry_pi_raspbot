SUBDIRS = MOTOR_DRIVER IR_SENSOR HC-SR04
MODULE_LIST = MOTOR_DRIVER/motor.ko IR_SENSOR/ir_avoid.ko HC-SR04/sr04.ko
SIGN = $(PWD)/sign_modules.sh
.PHONY: $(SUBDIRS) install clean
all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done
	for module_to_sign in $(MODULE_LIST); do \
		$(SIGN) $$module_to_sign; \
	done
install:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install; \
	done
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
