// =============================================================================
// trending_topics.dart  —  Horizontal trending keywords list
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../../core/theme/app_theme.dart';

class TrendingTopics extends StatelessWidget {
  final AsyncValue<List<Map<String, dynamic>>> trending;
  const TrendingTopics({super.key, required this.trending});

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            const Icon(Icons.whatshot, size: 18, color: Colors.orange),
            const SizedBox(width: 6),
            Text('Trending Now',
                style: theme.textTheme.titleSmall?.copyWith(
                    fontWeight: FontWeight.w700)),
          ],
        ),
        const SizedBox(height: 8),
        SizedBox(
          height: 38,
          child: trending.when(
            loading: () => const Center(
                child: SizedBox(
                    width: 20, height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2))),
            error: (_, __) => const SizedBox.shrink(),
            data: (topics) => ListView.separated(
              scrollDirection: Axis.horizontal,
              itemCount: topics.length,
              separatorBuilder: (_, __) => const SizedBox(width: 8),
              itemBuilder: (context, i) {
                final topic = topics[i];
                final isHot = topic['badge'] == 'hot';
                
                return Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                  decoration: BoxDecoration(
                    color: isDark ? AppColors.darkCard : AppColors.lightCard,
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(
                      color: isDark ? AppColors.darkBorder : AppColors.lightBorder,
                      width: 0.5,
                    ),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      if (isHot) ...[
                        const Text('🔥 ', style: TextStyle(fontSize: 12)),
                      ] else ...[
                        const Text('📈 ', style: TextStyle(fontSize: 12)),
                      ],
                      Text(
                        topic['keyword']!,
                        style: theme.textTheme.bodySmall?.copyWith(
                            fontWeight: FontWeight.w600,
                            color: isDark ? Colors.white : Colors.black87),
                      ),
                    ],
                  ),
                );
              },
            ),
          ),
        ),
      ],
    );
  }
}
