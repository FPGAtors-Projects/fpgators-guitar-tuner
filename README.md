# FPGA Guitar Tuner (FPGAtors)

A club project to build a real-time guitar tuner on the Digilent Zybo Z7-10 and compare FFT implementations written in C/C++ HLS and pure RTL.

The project combines audio acquisition, embedded software, DSP, hardware verification, and performance/resource/power analysis. All club members are welcome, regardless of experience.

## Current Status — October 26, 2026

The embedded platform and codec-control path are working. Audio capture and FFT processing are not implemented yet.

| Component | Status |
|---|---|
| Vivado hardware platform targeting Zybo Z7-10 | Implemented; bitstream programmed successfully |
| Standalone Cortex-A9 platform in Vitis 2024.2 | Built and used to run the application |
| Remote JTAG through an SSH reverse tunnel | Working |
| Application debugging and breakpoints | Working |
| Hello World through the local USB-UART connection | Verified |
| AXI GPIO for buttons, switches, and LEDs | Included in the block design; functional testing remains |
| AXI IIC connection to SSM2603 codec | Implemented; initialization sequence runs and reports completion |
| Codec register-transfer error/ACK checks | Review before treating printed status as full verification |
| Codec audio clocks and I²S capture | Not implemented |
| Audio capture into DDR | Not implemented |
| Software pitch estimator / reference FFT | Planned |
| HLS and handwritten RTL FFTs | Planned |
| Real-time tuner and comparative benchmarks | Planned |
| Fresh-clone project reconstruction | Not yet verified |

The present codec application demonstrates configuration only. Successful initialization messages do not establish that audio samples are being captured.

## Platform and Toolchain

| Setting | Current configuration |
|---|---|
| Board | Digilent Zybo Z7-10, Zynq-7000 |
| Tools | Vivado / Vitis 2024.2 |
| Hardware project | `platform_project/platform_project.xpr` |
| Hardware handoff | Fixed XSA including the bitstream |
| Vitis platform component | `platform` |
| Processor | `ps7_cortexa9_0` |
| OS/domain | Standalone / bare metal |
| Console stdin/stdout | `ps7_uart_1` |
| UART terminal | 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control |
| Accelerator integration | Ordinary Vivado IP, exported from HLS or packaged from RTL |

This project does not currently use PetaLinux, XRT, or the Vitis accelerated-application runtime flow. FFT accelerators will be integrated into a fixed Vivado design and controlled by a standalone ARM application.

The current target is the Zybo Z7-10. A ZedBoard port is a possible future extension, not a validated configuration; its board settings and audio hardware must be handled separately.

## Audio Architecture

Audio will enter through the Zybo's microphone or line-input jack and be digitized by the onboard SSM2603 audio codec. The planned audio path does not use the Zynq XADC.

The codec's digital audio interface is connected to the programmable logic, not directly to a PS audio peripheral.

### Implemented control path

```text
Cortex-A9 application
        ↓ AXI
AXI IIC controller in PL
        ↓ I²C
SSM2603 configuration registers
```

The external interface is `IIC_0`:

| Signal | Top-level port | FPGA pin | I/O standard |
|---|---|---|---|
| Codec SCL | `IIC_0_scl_io` | N18 | LVCMOS33 |
| Codec SDA | `IIC_0_sda_io` | N17 | LVCMOS33 |

### Planned capture and processing path

```text
Microphone / conditioned line-level source
        ↓
SSM2603 ADC
        ↓ I²S
PL audio receiver + clocking + buffering
        ↓
AXI DMA → PS DDR
        ↓
CPU reference processing or selected FPGA FFT
        ↓
Fundamental-frequency estimator
        ↓
Note / cents reporting and tuning indicators
```

Clock generation, codec settings, sample format, and receiver timing must be designed as one consistent configuration. Writing the codec registers does not generate MCLK, BCLK, or frame clocks.

The guitar input/front-end arrangement is still to be selected and validated. A cable adapter alone is not the project's completed analog-interface design.

## Repository Layout

```text
.
├── README.md
├── LICENSE
├── .gitignore
├── platform_project/
│   ├── platform_project.xpr
│   └── platform_project.srcs/
│       ├── constrs_1/imports/fpgators-guitar-tuner/
│       │   └── Zybo-Z7-Master.xdc
│       └── sources_1/bd/design_1/
│           ├── design_1.bd
│           └── ip/...
└── vitis_dev/
    ├── hello_world/
    │   ├── vitis-comp.json
    │   └── src/
    │       ├── helloworld.c
    │       ├── platform.c
    │       ├── platform.h
    │       ├── CMakeLists.txt
    │       ├── UserConfig.cmake
    │       └── ...
    └── scripts/
        └── recreate_project.tcl
```

The imported XDC under `platform_project.srcs/constrs_1/` is authoritative. The unused root-level master XDC is ignored.

Generated BSPs, FSBL outputs, Vitis platform files, caches, implementation runs, binaries, and XSA exports are excluded from normal source commits. Save deliberate BSP/FSBL customizations as reproducible source/configuration before regenerating them.

## Building and Running

This workflow has worked in the development workspace. Rebuilding from a clean clone remains a validation task.

1. Install/select the Zybo Z7-10 board files and Vivado/Vitis 2024.2.
2. Open `platform_project/platform_project.xpr`.
3. Confirm that all source references resolve, validate the block design, and generate output products and the HDL wrapper as required.
4. Run synthesis, implementation, and bitstream generation.
5. Export a fixed XSA including the bitstream.
6. Create a Vitis platform component named `platform`, using the exported XSA and a standalone domain on `ps7_cortexa9_0`.
7. Confirm the BSP console is assigned to `ps7_uart_1` and build the platform.
8. Create or restore the application using `vitis_dev/hello_world/src/`. Check component/CMake settings for machine-specific paths.
9. Build the application and configure the hardware debug connection.
10. Use the matching generated PS7 initialization in the JTAG launch flow before downloading/running the application. Do not manually add `ps7_init()` to the normal Hello World startup path.
11. Open the local serial terminal before resuming the application.

A reconstruction script is included at `vitis_dev/scripts/recreate_project.tcl`. Its relative paths and any references to generated checkpoints must be reviewed and tested before treating it as a portable one-command build. Do not move it without reviewing its path assumptions.

## Remote Development on the ECE Server

The board is physically connected to the developer's local computer. Vivado/Vitis run on `ece-lnx-01.ad.ufl.edu`.

```text
Local Zybo USB-JTAG
        ↕
Local hw_server:3121
        ↕ SSH reverse tunnel
ECE server localhost:43121
        ↕
Vitis / XSCT
```

### Local computer

Start the local hardware server, preferably matching 2024.2:

```bash
hw_server -s tcp::3121
```

The executable location differs between Windows/Linux and installation types. Then open a second local terminal and establish the tunnel:

```bash
ssh -N -o ExitOnForwardFailure=yes -o ServerAliveInterval=30 -o ServerAliveCountMax=3 -R 127.0.0.1:43121:127.0.0.1:3121 YOUR_UF_USERNAME@ece-lnx-01.ad.ufl.edu
```

Keep both processes running. Choose a different unused remote port if `43121` is occupied. The IDE must run on the same remote host where this listener is created.

### ECE server

Select `127.0.0.1:43121` as the Vitis hardware-server endpoint. For manual target inspection:

```bash
source /apps/xilinx/Vitis/2024.2/settings64.sh
xsct
```

```tcl
connect -url tcp:127.0.0.1:43121
targets
```

Target numbers may change between sessions. Disconnect manual debugger clients before launching a competing Vitis debug session.

### Board and UART

- Select JTAG boot mode while powered off, then power-cycle before a JTAG development session.
- Read UART on the local computer using the board's USB serial port.
- UART settings: `115200`, `8N1`, no flow control.
- The SSH hardware-server tunnel does not forward the serial port.
- Open the terminal before running past the print statements.

## Learning Path and FFT Comparison

### 1. Reference model and CPU baseline

- Develop a Python/NumPy/SciPy or MATLAB model.
- Add an ARM software FFT baseline for on-board timing comparisons.
- Establish test vectors, sample format, windowing, and pitch-estimation criteria.
- Select sample rate and transform length using both accuracy and observation-latency requirements.

### 2. HLS implementations

- Implement FFT variants in C/C++ using explicit fixed-point formats.
- Explore radix, hardware reuse, pipelining, unrolling, memory banking, and dataflow.
- Verify with C simulation and C/RTL co-simulation.
- Export as Vivado IP for integration in the fixed hardware design.
- Label vendor FFT-library/IP results separately from handwritten HLS algorithm implementations.

### 3. Handwritten RTL

- Study FFT architectures, including [Dr. Wong's FFT implementation notes](https://tanfwong.github.io/rfsoc_notes/ch8/impl.html).
- Build a custom Verilog/SystemVerilog FFT.
- Verify butterfly arithmetic, twiddle addressing, scaling, output ordering, and stream handshaking against shared vectors.

### Common comparison contract

All variants should document transform length, input/output precision, twiddle precision, scaling, rounding, overflow behavior, output order, and frame protocol. Prefer one FFT implementation per bitstream, with the same surrounding capture/DMA system.

Compare:

- Numerical error and tuning accuracy.
- Kernel latency and sustained throughput.
- End-to-end offload time, including data movement and control.
- Speedup relative to the stated ARM software baseline.
- Post-implementation LUT, FF, DSP, and BRAM utilization.
- Timing closure at the stated clock frequency.
- Estimated/measured power and energy per frame, with measurement boundaries documented.
- Design effort and verification complexity.

FPGA resource counts are the project's area proxies, not custom-silicon area measurements. Distinguish FFT-only results from whole-system results, and estimates from physical measurements.

## Milestones

- [x] Create the fixed hardware platform and standalone application.
- [x] Establish remote JTAG debugging and local UART output.
- [x] Run the codec I²C initialization application.
- [ ] Review codec transfer checks and recording configuration.
- [ ] Validate clean-clone reconstruction.
- [ ] Bring up codec clocks and the I²S receive path.
- [ ] Capture and inspect a known tone in DDR.
- [ ] Implement the reference model and ARM software baseline.
- [ ] Implement and verify the first HLS FFT.
- [ ] Implement and verify alternative HLS and handwritten RTL FFTs.
- [ ] Add continuous capture with ping-pong/frame buffering.
- [ ] Integrate fundamental estimation, note classification, and tuning feedback.
- [ ] Benchmark performance, resources, and power under controlled workloads.
- [ ] Demonstrate live guitar tuning.

The immediate next milestone is reliable audio capture—not FFT acceleration. Start with a known test tone, verify sample alignment and effective sample rate, and check for clipping before using live guitar audio. The fundamental estimator must account for harmonics rather than simply selecting the largest FFT bin.

## Target Notes — Standard Guitar Tuning

Nominal targets using A4 = 440 Hz:

| String | Note | Frequency |
|---|---|---|
| 6 | E2 | 82.41 Hz |
| 5 | A2 | 110.00 Hz |
| 4 | D3 | 146.83 Hz |
| 3 | G3 | 196.00 Hz |
| 2 | B3 | 246.94 Hz |
| 1 | E4 | 329.63 Hz |

## References and Starting Points

- [Codec I²C bring-up example](https://github.com/Rennaafi/zybo-i2c-ssm2603-bringup): basis for the current control-path application; not a complete audio receiver.
- [Digilent Zybo Z7 DMA Audio Demo](https://digilent.com/reference/programmable-logic/zybo-z7/demos/dma-audio): planned reference for capture/playback. Use the Z7-10 variant; the 2022.1 release requires adaptation to this project's 2024.2 toolchain.
- [Zybo Z7 reference manual](https://digilent.com/reference/programmable-logic/zybo-z7/reference-manual).
- [Unified Vitis standalone platform tutorial](https://byu-cpe.github.io/ecen625/vitis-tutorial/).
- [Dr. Wong's RFSoC FFT notes](https://tanfwong.github.io/rfsoc_notes/ch8/impl.html): algorithm/architecture study reference, not a drop-in Zybo platform.

## Participation

No prior FPGA experience is required. Members can contribute to audio acquisition, DSP modeling, HLS, RTL, embedded software, verification, benchmarking, documentation, or demonstrations. Roles can rotate as members learn.
