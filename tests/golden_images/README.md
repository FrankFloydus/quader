# Golden Images

This directory is intentionally metadata-only for task #11 A53. Do not add
placeholder images, screenshots, blank PNGs, or machine-local captures.

Golden baselines may be added only after renderer authority approves a stable
request-scoped capture path that reads final display-space `RGBA8` bytes from
Crimson, not a whole-window screenshot. Each future baseline must include
adjacent metadata that records:

- scene name
- renderer backend
- capture color space
- viewport size
- tolerance values
- update reason

The default CTest suite currently runs only synthetic in-memory comparator tests
through the `golden_image_compare_tests` target. Runtime capture and baseline
comparison tests must remain opt-in until real baselines are reviewed.
