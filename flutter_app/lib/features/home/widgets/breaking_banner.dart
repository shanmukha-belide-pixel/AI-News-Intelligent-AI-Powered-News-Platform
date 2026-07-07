// =============================================================================
// breaking_banner.dart  —  Featured breaking news banner widget
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import '../../../../core/theme/app_theme.dart';
import '../../news/models/article_model.dart';
import 'package:go_router/go_router.dart';

class BreakingBanner extends StatelessWidget {
  final List<ArticleModel> articles;
  const BreakingBanner({super.key, required this.articles});

  @override
  Widget build(BuildContext context) {
    if (articles.isEmpty) return const SizedBox.shrink();
    
    // Pick the most recent breaking article
    final article = articles.first;
    final theme   = Theme.of(context);
    final isDark  = theme.brightness == Brightness.dark;

    return GestureDetector(
      onTap: () => context.push('/article/${article.id}'),
      child: Container(
        padding: const EdgeInsets.all(14),
        decoration: BoxDecoration(
          color: Colors.red.withOpacity(0.1),
          borderRadius: BorderRadius.circular(16),
          border: Border.all(color: Colors.red.withOpacity(0.3), width: 1),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Container(
                  width: 8, height: 8,
                  decoration: const BoxDecoration(
                    color: Colors.red,
                    shape: BoxShape.circle,
                  ),
                ).animate(onPlay: (c) => c.repeat(reverse: true))
                 .fade(begin: 1, end: 0.2, duration: 600.ms),
                const SizedBox(width: 6),
                Text(
                  'BREAKING NEWS',
                  style: theme.textTheme.labelMedium?.copyWith(
                    color: Colors.red,
                    fontWeight: FontWeight.w800,
                    letterSpacing: 1.0,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 6),
            Text(
              article.title,
              maxLines: 2,
              overflow: TextOverflow.ellipsis,
              style: theme.textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.w800,
                height: 1.3,
              ),
            ),
            const SizedBox(height: 4),
            Row(
              children: [
                Text(
                  article.publisher,
                  style: TextStyle(
                    color: isDark ? AppColors.darkSubtext : AppColors.lightSubtext,
                    fontSize: 11,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                const Spacer(),
                const Text(
                  'Read Live →',
                  style: TextStyle(
                    color: Colors.red,
                    fontSize: 11,
                    fontWeight: FontWeight.w700,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
