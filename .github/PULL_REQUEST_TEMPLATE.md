<!-- Please fill out the following pull request template for non-trivial changes to help us process your PR faster and more efficiently. -->
---
## Basic Info
| Info | Please fill out this column |
| ------ | ----------- |
| Ticket(s) this addresses | (add tickets here #1) |
| ROS 2 distro | Jazzy |
---

## Description of contributions
<!--
* I added this neat new feature
* Also fixed a typo in a parameter name in ros_package_name
-->

## Description of how this change was tested
<!--
* I wrote unit tests that cover 90%+ of changes and extensively tested on my physical robot platform for 1 week
* I wrote unit tests and tested in simulation for 10 minutes
-->

## Build verification
- [ ] `colcon build --packages-select <pkg> --symlink-install` succeeds
- [ ] `colcon test` passes
- [ ] `package.xml` dependencies updated if new deps were added

## Breaking changes
<!-- Does this change any public API, topic/service/action names, message definitions, or launch args? -->

Breaking? (Yes/No):

<!-- If yes, describe what breaks and any migration steps -->

## Related PRs / dependencies
<!-- e.g. depends on rviz_default_plugins#42, should merge after ornl_ros_boilerplate#17 -->

## Checklist
- [ ] Docs / README updated if behavior changed
- [ ] Self-reviewed the diff
