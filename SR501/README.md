# HC-SR501 PIR Sensor Driver for i.MX6Ull

This project provides a simple Linux character driver for the HC-SR501 PIR motion sensor on an i.MX6Ull platform.

-----

## 1\. Device Tree Configuration

To make the kernel aware of the sensor, you need to add a description to your board's device tree file (e.g., `imx6ull-14x14-evk.dts`).

**A. Add the SR501 node**

Add the following snippet under the root node (`/`) of your `.dts` file. This defines the sensor, its `compatible` string for driver matching, and the GPIO it uses.

```dts
sr501 {
    compatible = "100ask,sr501";
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_sr501>;
    gpios = <&gpio1 4 GPIO_ACTIVE_HIGH>; 
    status = "okay";
}
```

**B. Add the pin control (pinctrl) group**

Add the following snippet inside the `iomuxc` node to configure the pin's function.

```dts
pinctrl_sr501: sr501_grp {
    fsl,pins = <
        /* MUX_MODE        CONFIG */
        MX6UL_PAD_GPIO1_IO04__GPIO1_IO04    0x17059 
    >;
};
```

**C. Recompile and Update**

After saving the changes, recompile the device tree file to generate the `.dtb` binary and update it on your board's boot partition. Reboot the board for the changes to take effect.

-----

## 2\. Deployment and Execution

Follow these steps to compile, deploy, and run the driver and test application.

**Step 1: Compile the Code**

Use the provided `Makefile` to build the driver and the test application. From your project directory, run:

```bash
make
```

This will generate two files:

  - `sr501_drv.ko`: The kernel driver module.
  - `test_sr501`: The user-space test application.

**Step 2: Deploy to Target**

Copy the `sr501_drv.ko` and `test_sr501` files to your i.MX6Ull board, for example, using `scp`.

**Step 3: Load the Driver**

On the board's terminal, load the kernel module:

```bash
insmod sr501_drv.ko
```

You can verify that the driver loaded successfully by checking the kernel log and the device file:

```bash
dmesg | tail
ls /dev/sr501
```

You should see messages from the driver and the `/dev/sr501` device node.

**Step 4: Run the Test Application**

Execute the test program:

```bash
./test_sr501
```

The application will wait for sensor input, displaying the following message:

```
Device /dev/sr501 opened successfully. Waiting for motion...
Blocking on read()... Trigger the sensor.
```

**Step 5: Trigger the Sensor**

Wave your hand in front of the HC-SR501 sensor. The test application will immediately detect the event and print a confirmation message:

```
Event detected! read() returned 4 bytes. Value = 1

Blocking on read()... Trigger the sensor.
```

**Step 6: Unload the Driver**

Press `Ctrl+C` to stop the test application. To remove the driver from the kernel, run:

```bash
rmmod sr501_drv
```