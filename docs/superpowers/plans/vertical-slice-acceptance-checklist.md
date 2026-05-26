# Vertical Slice Acceptance Checklist

- [ ] **Completable cold.** Boot. New profile. Maze select -> Nature. Reach the gardener, take the dialog (rain_boots granted). Use the seed on the withered plant. Walk through mud. Use rain_boots on the locked gate. Reach the goal tile.
- [ ] **Resume correctness.** Mid-slice, close the app. Reopen. State resumes at last checkpoint.
- [ ] **Reset safety.** Trigger maze reset. Confirm profile-scope rain_boots and outfit_worn are preserved while maze state is cleared.
- [ ] **Soft-lock resistance.** Use the seed on the locked gate by mistake. Confirm seed is not consumed.
- [ ] **Fully localized.** UI strings route through `t()` for slice-facing text.
- [ ] **Accessibility minimum.** Controls can be rebound, text speed can be changed, and puzzle cues are not color-only.
- [ ] **Save robustness.** Corrupt the DB manually. Reboot. Recovery modal appears and a backup exists.
- [ ] **Performance.** Observe 60 fps for normal play on the target machine.
- [ ] **Quiet exit.** Quit during play. Reopen. SQLite opens cleanly and the latest checkpoint is present.

Manual playthrough is still required before declaring the slice accepted.
