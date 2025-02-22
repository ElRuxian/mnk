// Barebones implementation of a range_formatter, to be
// used while the stdlibc++ implementation is not available

#include <format>

#include <version>
#if !(defined(__cpp_lib_format) && __cpp_lib_format >= 202207L)

#include <ranges>

template <std::ranges::input_range Range>
struct range_formatter {
        const Range &range;
};

template <std::ranges::input_range Range>
range_formatter(const Range &) -> range_formatter<Range>;

template <std::ranges::input_range Range, typename CharT>
struct std::formatter<range_formatter<Range>, CharT> : formatter<CharT> {

        template <typename FormatContext>
        static auto
        format(const range_formatter<Range> &rf, FormatContext &ctx)
        {
                auto out = ctx.out();
                auto it  = rf.range.begin();
                auto end = rf.range.end();

                *out++ = '[';

                if (it != end) {
                        out = format_to(out, "{}", *it);
                        ++it;
                }
                for (; it != end; ++it)
                        out = format_to(out, ", {}", *it);

                *out++ = ']';

                return out;
        }
};

template <std::ranges::input_range Range, typename CharT>
struct std::formatter<Range, CharT>
        : std::formatter<range_formatter<Range>, CharT> {

        template <typename FormatContext>
        static auto
        format(const Range &range, FormatContext &ctx)
        {
                return std::formatter<range_formatter<Range>, CharT>::format(
                    range_formatter{ range }, ctx);
        }
};

#endif
