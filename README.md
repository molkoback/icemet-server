# ICEMET Server
Digital hologram processing and cloud droplet analysis software for ICEMET project.

## Building
Build requirements: Git, Python3, fmt, libarchive, MariaDB, OpenCV, yaml-cpp
```bash
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

## Citation
E. O. Molkoselkä, V. A. Kaikkonen and A. J. Mäkynen, "Measuring Atmospheric Icing Rate in Mixed-Phase Clouds Using Filtered Particle Data," in *IEEE Transactions on Instrumentation and Measurement*, vol. 70, pp. 1-8, 2021, Art no. 7001708, doi: 10.1109/TIM.2020.3035562.
```bibtex
@article{molkoselka2021measuring,
	author={E. O. Molkoselk{\"a} and V. A. {Kaikkonen} and A. J. {M{\"a}kynen}},
	journal={IEEE Transactions on Instrumentation and Measurement},
	title={Measuring Atmospheric Icing Rate in Mixed-Phase Clouds Using Filtered Particle Data},
	year={2021},
	volume={70},
	number={},
	pages={1-8},
	doi={10.1109/TIM.2020.3035562}
}
```

## License
ICEMET Server is licensed under the MIT license, please see LICENSE for details.
