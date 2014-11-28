################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/httpclient.c \
../src/linkparser.c \
../src/main.c \
../src/threadmanager.c 

OBJS += \
./src/httpclient.o \
./src/linkparser.o \
./src/main.o \
./src/threadmanager.o 

C_DEPS += \
./src/httpclient.d \
./src/linkparser.d \
./src/main.d \
./src/threadmanager.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O3 -Wall -c -fmessage-length=0 -D_FILE_OFFSET_BITS=64 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


