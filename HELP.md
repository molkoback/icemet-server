# Help
## Usage
`icemet-server [options] config.yaml`

### Options
- `-h` Print this help message and exit.
- `-V` Print version info and exit.
- `-p` Particles only.
- `-s` Stats only. Particles will be fetched from the database.
- `-Q` Quit after processing all available files.
- `-d` Enable debug messages.

## Config
Config template: [icemet-server.yaml](etc/icemet-server.yaml)

### Files
- `path_watch <str>` Path to watch. ICEMET Server looks for readable files from this directory and its subdirectories recursively.
- `path_results <str>` Path where ICEMET Server saves the processed results.
- `save_results <str>` Which results will be saved.
  - `o` Original images.
  - `p` Preprocessed images.
  - `m` Minimum images.
  - `r` Reconstructed particles.
  - `t` Thresholded particles
  - `v` Preview images.
- `save_empty <bool>` Save empty files.
- `save_skipped <bool>` Save skipped files.
- `type_results <str>` Result image file types.
- `type_results_lossy <str>` Lossy result image file types (for preview images).

### SQL server
- `sql_host <str>` SQL server host.
- `sql_port <int>` SQL server port.
- `sql_user <str>` SQL server username.
- `sql_passwd <str>` SQL server password.
- `sql_database <str>` SQL server database.
- `sql_table_particles <str>` Particles table name.
- `sql_table_stats <str>` Stats table name.
- `sql_table_meta <str>` Meta table name. If empty, the meta information will not be written.

###  Preprocessing
- `img_(x|y|w|h) <int>` Image cropping rectangle.
- `img_rotation <int>` Image rotation in degrees.
- `empty_th_original <int>` Original images with max-min value smaller than this will be marked as empty.
- `empty_th_preproc <int>` Preprocessed images with max-min value smaller than this will be marked as empty.
- `empty_th_recon <int>` After preprocessing, a minimum image is created with 10x sparser step than `holo_dz`. If the max-min value of the minimum image is smaller than the value, the image will be marked as empty.
- `noisy_th_recon <int>` After preprocessing, a minimum image is created with 10x sparser step than `holo_dz`. The minimum image is binarized and contours will be extracted. If the number of contours is higher than the value, the image will be marked as skipped.
- `bgsub_stack_len <int>` Background subtraction stack length. 0 for no background subtraction.
- `filt_lowpass <int>` Super-gaussian lowpass filter frequency. 0 for no filter.

### Reconstruction
 - `holo_(z0|z1|dz) <float>` Hologram reconstruction start, end and step in meters.
 - `holo_pixel_size <float>` Camera single pixel size in meters.
 - `holo_lambda <float>` Laser wavelength in meters.
 - `holo_distance <float>` Distance between the camera and laser in meters for uncollimated beams. 0 for collimated beams.
 - `recon_step <int>` The number of frames in each reconstruction batch. Can be used to limit the memory usage.
 - `focus_step <int>` The number of frames between frames that will be used in the focusing. Can be used to speed up the focusing.
 - `segment_th_factor <float>` Segment threshold factor *f*:
*th = f · Median(preproc)*
 - `segment_size_(min|max) <int>` Size range of the segments (width or height) in pixels.
 - `segment_size_small <int>` Segments smaller than this will be focused using a minimum based method.
 - `segment_pad <int>` Padding around the segments in pixels.
 - `img_ignore_(x|y) <int>` Image border area in pixels that will be ignored.

### Analysis
 - `segment_scale <float>` The size (width or height) the smaller segments will be upscaled to in pixels.
 - `particle_th_factor <float>` Particle threshold factor *f*:
*bg = Median(preproc)*
*min = Min(segm)*
*th = bg - f · (bg - min)*
 - `diam_corr <bool>` Enable particle diamater correction.
 - `diam_corr_(d0|d1) <float>` Diameter correction start and end sizes.
 - `diam_corr_(f0|f1) <float>` Diameter correction start and end factors. The factor changes linearly between *d0* and *d1*.

### Statistics
 - `stats_time <int>` Time between stats points in seconds.
 - `stats_frames <int>` Fixed number of frames used for stats calculation. Required only in the stats only mode.
 - `particle_z_(min|max) <float>` Particle min/max z-position for stats calculation.
 - `particle_diam_(min|max) <float>` Particle min/max size for stats calculation.
 - `particle_diam_step <float>` Bin step used in MVD calculation.
 - `particle_circ_(min|max) <float>` Particle min/max Heywood circularity factor for stats calculation:
*circ = perim / (2 · Sqrt(π · area))*
 - `particle_dynrange_(min|max) <int>` Particle min/max dynamic range for stats calculation.

### OpenCL
 - `ocl_device <str>` OpenCL device.
