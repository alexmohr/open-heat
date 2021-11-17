format_dir=${1:-.}
# shellcheck disable=SC2038
find "$format_dir/src" -regex '.*\.\(cpp\|hpp\)$' \
  -not -path "${format_dir}"'/build*' \
  | xargs -n 1 clang-format -i