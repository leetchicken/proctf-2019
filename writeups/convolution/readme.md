# Convolution
Service allows users to put images to convolute and get the time it took the service to process the images. Part of the service is written on custom vector assembler, similar to ISA of modern GPUs

## Functionality

### GET /list
Gets a list of IDs of convolution kernels parameters. The response is a plain text with the IDs:
```
jpci-7xfh-fkgi
nwd2-6iem-wfwa
4kmf-2dli-mn58
5r83-xjla-5j7c
vsvp-937r-lo3e
rpon-qspu-cy9u
52cv-vu95-tghm
6h4t-dl16-bkuj
q78m-za9z-m71s
09lw-ykdo-3zqp
```

### POST /process?kernel-id=\<id of kernel parameters\>
Puts the image(images) to convolute with specified convolution kernel parameters. Content may have one and more images. The response is JSON which contains times it took the service to process every channel of every image. Here is an example of response of request to process three images:
```
{ "3GYZGTI9NCHYMF1LYC1LQ9FRGLX6R20T": { "red_channel" : 76836, "green_channel" : 75710, "blue_channel" : 74550, "alpha_channel" : 71770 }, "RO450VSGGNUHH9JKZGL3YKDFXO14CN1L": { "red_channel" : 502162, "green_channel" : 453734, "blue_channel" : 448513, "alpha_channel" : 433320 }, "SXONRIYPYM1ZR7L461D51UR4VSM4M6X0": { "red_channel" : 62356, "green_channel" : 61912, "blue_channel" : 60816, "alpha_channel" : 58642 }}
```
If ID of convolution kernel parameters is signed(signature is expected in HTTP headers) service saves processed images on the disk.

### GET /get-image?name=\<image name\>
Gets processed and saved image from disk. Image name must be signed, signature is expected in HTTP headers. This functionality needed to check functionality of the service. It is unavailable for teams because it is possible to restore convolution kernel parameters, i.e flag, from processed image.

### POST /add-kernel?kernel-id=\<id of kernel parameters\>&kernel=\<kernel parameters\>
Adds new convolution kernel parameters. ID is a string, maximum length is 32. Convolution kernel parameters is a string, length must be exact 32. ID must be signed(signature is expected in HTTP headers). Checker uses this functionality to post flags as convolution kernel parameters. Example:
```
http://somehost.com/add-kernel?kernel-id=R98PQO4U&kernel=MUUOOOKGPYQCGGVHKTLUCGOQCEULRDL=
```

### GET /get-kernel?kernel-id=\<id of kernel parameters\>
Gets convolution kernel parameters by ID. ID must be signed(signature is expected in HTTP headers). Response is a plain text which contain convolution kernel paramaters

### Note
Putting/getting convolution kernel parameters, getting convoluted images are allowed only for those who provide valid digital signature. It intended only checker is allowed for that because only checker has private key.

## Convolution
Convolution takes 3x3 pixels blocks from source image and convolutes them to 1 pixel of destination image and perform it for every channel separetly. There is only one convolution kernel, but it has 32 parameters each of them is one byte long. So it is 32 bytes long as a flag.
Here is 3x3 block of source image, each pixel marked with index from 0 to 31:

![img0](img0.png)

Kernel formula is:

![formula](formula.png)

where 'c' is channel index(0 - red, 1 - green, 2 - blue, 3 - alpha), 'p' - pixels value(one byte), 'k' - kernel parameters.

## Vector assembler
For convolution program written on custom assembler is used. It is similar to AMD GPU ISA. Compiled program written on this assembler called kernel, dont mess with convolution kernel. Each vector register has 8 lanes because vector instructions and registers are implemented by using AVX2 and SSE4. As in modern GPUs some of lanes may be deactivated during execution because of flow control instructions. Compiler generates two variantes of code: AVX2 variant and SSE4 variant. On the kernel startup all of 8 lanes are active and AVX2 variant is used, but when last 4 lanes are inactive flow control switchs to SSE4 variant, otherwise AVX2 variant is used.

### Vuln
To exploit posibilties of vectorization kernel(compiled program) that performs convolution loads data in vector register as follows:

![24x3](24x3.png)

Where Vx_y is a vector registers, x - register index, ie v0, v1, v2, y - lane index. One kernel execution performs convolution of 8 3x3 blocks, thats why destination image's width aligned by 24 and height aligned by 3.
Teams are not allowed to downloaded processed images, but there is side channel - teams can post special images and by comparing times restore kernel parameters, ie flag. This happens because switches between AVX2 and SSE4 variants of code are expensive because xsave x86 instruction is used. 

Naive way to steal 0th symbol of flag is to prepare 39 images for each possible symbol in flag as follows:

Symbol .
![dot](dot.png)
Symbol 0
![0](0.png)
Symbol 1
![1](1.png)
and so on to 9
Symbol =
![=](=.png)
Symbol A
![A](A.png)
Symbol B
![B](B.png)
and so on to Z
Symbol _
![_](_.png)

Next step is to submit them all to process, after you got timings you need to look to timings for red channel, for example you got this:

| .  | 0  | 1  | 2  | 3  | 4  | 5  | 6  | 7  | 8  | 9  | =  | A  | B  | C  | D  | E  | F  | G  | H  | I  | J  | K  | L  | M  | N  | O  | P  | Q  | R  | S  | T  | U  | V  | W  | X  | Y  | Z  | _ |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 138088  | 130454  | 129942  | 129706  | 129626  | 129733  | 129659  | 129760  | 129799  | 129785  | 129635  | 135234  | 130276  | 129675  | 129760  | 129825  | 129686  | 129990  | 130020  | 129705  | 130046  | 129681  | 131270  | 129686  | 130331  | 129992  | 129788  | 130114  | 129955  | 129990  | 129806  | 130039  | 129615  | 97530  | 97604  | 97468  | 97480  | 97675  | 97639 |

As you can see there is a drop between 'U' and 'V' symbols, that means that first symbol of flag is 'V'.

Why so?
Here is an example of how 2nd pixel in each block processed(for remain pixels processing is similar):
```
# load address of kernel parameters
0 s_load s1q, 16, s0q
# load 8 bytes of kernel parameters
1 s_load s1q, 0, s1q
# because we process 2nd byte shift left by 16
2 s_shr s1q, s1q, 16
# ... and left only byte, so s1q contains exact only one 2nd parameter
3 s_and s1q, s1q, 0xff
# move parameter to vector register
4 v_mov v11, s1
# compare with pixel
5 v_cmp_eq_u32 v2, v11
6 s_mov s2, vcc
7 v_cmp_gt_u32 v2, v11
8 s_or exec, s2, vcc
# if compare is true perform calculations below
9 v_add_u32 v12, v12, v2
10 v_add_f32 v13, v13, 1.0
# restore exec register
11 s_mov exec, 0xff
```
Transition between AVX2 and SSE4 therefore penalty may happen in 8th line if last 4 bits of exec register are zeroes and then another penalty happens in 11th line. 
Each pixel of 8 3x3 blocks except 0th we clear by value 0xff, that means that compare ![compare](compare.png) is true for all lanes because any symbol in flag is less that 0xff, so this lanes will be active and there will not be any transitions between AVX2 and SSE4 variants. 0th pixel of 0th, 1st, 2nd, 3rd, 5th, 6th and 7th 3x3 block we clear by value 0x0, that means that compare ![compare](compare.png) is false for 0th, 1st, 2nd, 3rd, 5th, 6th and 7th lanes. The key is 4th block, if compare is true exec mask equals to 0b00001000 and AVX2 variant is used, if false - exec mask equals to 0b00000000 and switch to SSE4 variant happens therefore penalty.

Next step is to repeat all this for the remain 31 bytes of kernel parameters, the only difference is that you need to select another pixel and channel:

| Byte index | Pixel index in block | Channel |
|---|---|-----|
| 0 | 0 | red |
| 1 | 1 | red |
| 2 | 2 | red |
| 3 | 3 | red |
| 4 | 4 | red |
| 5 | 5 | red |
| 6 | 6 | red |
| 7 | 7 | red |
| 8 | 0 | green |
| 9 | 1 | green |
| 10 | 2 | green |
| 11 | 3 | green |
| 12 | 4 | green |
| 13 | 5 | green |
| 14 | 6 | green |
| 15 | 7 | green |
| 16 | 0 | blue |
| 17 | 1 | blue |
| 18 | 2 | blue |
| 19 | 3 | blue |
| 20 | 4 | blue |
| 21 | 5 | blue |
| 22 | 6 | blue |
| 23 | 7 | blue |
| 24 | 0 | alpha |
| 25 | 1 | alpha |
| 26 | 2 | alpha |
| 27 | 3 | alpha |
| 28 | 4 | alpha |
| 29 | 5 | alpha |
| 30 | 6 | alpha |
| 31 | 7 | alpha |

### Fixing
Just remove xsave instruction between AVX2 and SSE4 switches

## Note
Tested only on Intel Coffee Lake Processors
