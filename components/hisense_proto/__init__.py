"""Read-only Hisense "XM" (F4 F5) bus protocol library.

Framing, the frame receiver, query frames (device type, version, status) and reply decoders.
It has no configuration of its own: hisense_ac and ac_setup load it with AUTO_LOAD. The frames
that change the AC state live in components/hisense_ac/hisense_command.{h,cpp}.

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

CODEOWNERS = ["@skydoy"]
