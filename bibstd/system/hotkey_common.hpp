#pragma once

namespace bibstd::system
{

///
/// Hotkey common class for all system implementations.
///
struct hotkey_common final
{
  // Typedefs
  enum class key
  {
    vk_left_button,
    vk_right_button,
    vk_middle_button,
    vk_x1_button,
    vk_x2_button,
    vk_backspace,
    vk_tab,
    vk_clear,
    vk_return,
    vk_pause,
    vk_capital,
    vk_escape,
    vk_space,
    vk_page_up,
    vk_page_down,
    vk_end,
    vk_home,
    vk_left,
    vk_up,
    vk_right,
    vk_down,
    vk_insert,
    vk_delete,
    vk_0,
    vk_1,
    vk_2,
    vk_3,
    vk_4,
    vk_5,
    vk_6,
    vk_7,
    vk_8,
    vk_9,
    vk_a,
    vk_b,
    vk_c,
    vk_d,
    vk_e,
    vk_f,
    vk_g,
    vk_h,
    vk_i,
    vk_j,
    vk_k,
    vk_l,
    vk_m,
    vk_n,
    vk_o,
    vk_p,
    vk_q,
    vk_r,
    vk_s,
    vk_t,
    vk_u,
    vk_v,
    vk_w,
    vk_x,
    vk_y,
    vk_z,
  };

  enum class key_modifier
  {
    alt,
    control,
    shift,
    alt_control,
    alt_shift,
    control_shift,
  };
};

} // namespace bibstd::system
