// =============================================================================
// news_card.dart  —  Premium glassmorphism news card
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:cached_network_image/cached_network_image.dart';
import 'package:timeago/timeago.dart' as timeago;
import 'package:haptic_feedback/haptic_feedback.dart';
import '../../../core/theme/app_theme.dart';
import '../../news/models/article_model.dart';

class NewsCard extends StatefulWidget {
  final ArticleModel article;
  final VoidCallback onTap;
  final Color? accentColor;

  const NewsCard({
    super.key,
    required this.article,
    required this.onTap,
    this.accentColor,
  });

  @override
  State<NewsCard> createState() => _NewsCardState();
}

class _NewsCardState extends State<NewsCard>
    with SingleTickerProviderStateMixin {
  bool _bookmarked = false;
  bool _liked      = false;
  late AnimationController _heartCtrl;

  @override
  void initState() {
    super.initState();
    _heartCtrl = AnimationController(
        vsync: this, duration: const Duration(milliseconds: 300));
  }

  @override
  void dispose() {
    _heartCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme    = Theme.of(context);
    final isDark   = theme.brightness == Brightness.dark;
    final article  = widget.article;
    final accent   = widget.accentColor ?? AppColors.primary;

    return GestureDetector(
      onTap: widget.onTap,
      child: AnimatedContainer(
        duration: const Duration(milliseconds: 200),
        decoration: BoxDecoration(
          color: isDark ? AppColors.darkCard : AppColors.lightCard,
          borderRadius: BorderRadius.circular(16),
          border: Border.all(
            color: isDark ? AppColors.darkBorder : AppColors.lightBorder,
            width: 0.5,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withOpacity(isDark ? 0.3 : 0.08),
              blurRadius: 12,
              offset: const Offset(0, 4),
            ),
          ],
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // ── Hero image ────────────────────────────────────────────────
            if (article.imageUrl.isNotEmpty)
              ClipRRect(
                borderRadius: const BorderRadius.vertical(top: Radius.circular(16)),
                child: Stack(
                  children: [
                    CachedNetworkImage(
                      imageUrl: article.imageUrl,
                      height: 180,
                      width: double.infinity,
                      fit: BoxFit.cover,
                      placeholder: (_, __) => Container(
                        height: 180,
                        color: isDark ? AppColors.darkBorder : AppColors.lightBorder,
                      ).animate(onPlay: (c) => c.repeat())
                       .shimmer(duration: 1200.ms, color: Colors.white.withOpacity(0.05)),
                      errorWidget: (_, __, ___) => Container(
                        height: 180,
                        color: isDark ? AppColors.darkBorder : AppColors.lightBorder,
                        child: const Icon(Icons.article_outlined, size: 48,
                            color: AppColors.darkSubtext),
                      ),
                    ),
                    // Gradient overlay at bottom of image
                    Positioned(
                      bottom: 0, left: 0, right: 0,
                      child: Container(
                        height: 60,
                        decoration: BoxDecoration(
                          gradient: LinearGradient(
                            begin: Alignment.bottomCenter,
                            end: Alignment.topCenter,
                            colors: [
                              (isDark ? AppColors.darkCard : AppColors.lightCard)
                                  .withOpacity(0.9),
                              Colors.transparent,
                            ],
                          ),
                        ),
                      ),
                    ),
                    // Category badge
                    Positioned(
                      top: 10, left: 10,
                      child: _CategoryBadge(
                          category: article.category, accent: accent),
                    ),
                    // Breaking badge
                    if (article.isBreaking)
                      Positioned(
                        top: 10, right: 10,
                        child: _BreakingBadge(),
                      ),
                    // Scope badge
                    Positioned(
                      bottom: 8, right: 10,
                      child: _ScopeBadge(scope: article.scope),
                    ),
                  ],
                ),
              ),

            // ── Content ───────────────────────────────────────────────────
            Padding(
              padding: const EdgeInsets.all(14),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  // Publisher row
                  Row(
                    children: [
                      if (article.publisherIcon.isNotEmpty)
                        CircleAvatar(
                          radius: 10,
                          backgroundImage: NetworkImage(article.publisherIcon),
                          onBackgroundImageError: (_,__) {},
                          backgroundColor: AppColors.darkBorder,
                        ),
                      const SizedBox(width: 6),
                      Text(article.publisher,
                          style: TextStyle(
                              color: accent,
                              fontSize: 12,
                              fontWeight: FontWeight.w600)),
                      const Spacer(),
                      Icon(Icons.access_time_rounded, size: 11,
                          color: isDark ? AppColors.darkSubtext : AppColors.lightSubtext),
                      const SizedBox(width: 3),
                      Text(_timeAgo(article.publishedAt),
                          style: TextStyle(
                              color: isDark ? AppColors.darkSubtext : AppColors.lightSubtext,
                              fontSize: 11)),
                    ],
                  ),
                  const SizedBox(height: 8),

                  // Headline
                  Text(article.title,
                      maxLines: 2,
                      overflow: TextOverflow.ellipsis,
                      style: theme.textTheme.titleMedium?.copyWith(
                          fontWeight: FontWeight.w700, height: 1.35)),
                  const SizedBox(height: 6),

                  // AI Summary (one-line)
                  if (article.aiSummary.isNotEmpty)
                    Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 10, vertical: 6),
                      decoration: BoxDecoration(
                        color: accent.withOpacity(0.08),
                        borderRadius: BorderRadius.circular(8),
                        border: Border(
                            left: BorderSide(color: accent, width: 2)),
                      ),
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Icon(Icons.auto_awesome, size: 13, color: accent),
                          const SizedBox(width: 6),
                          Expanded(
                            child: Text(article.aiSummary,
                                maxLines: 2,
                                overflow: TextOverflow.ellipsis,
                                style: theme.textTheme.bodySmall?.copyWith(
                                    color: isDark
                                        ? AppColors.darkText
                                        : AppColors.lightText)),
                          ),
                        ],
                      ),
                    ),
                  const SizedBox(height: 10),

                  // State label (if state news)
                  if (article.state.isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(bottom: 6),
                      child: Row(
                        children: [
                          const Icon(Icons.location_on, size: 12,
                              color: AppColors.darkSubtext),
                          const SizedBox(width: 4),
                          Text(article.state,
                              style: const TextStyle(
                                  fontSize: 11, color: AppColors.darkSubtext)),
                        ],
                      ),
                    ),

                  // Action row
                  Row(
                    children: [
                      // Reading time
                      Icon(Icons.schedule_rounded, size: 12,
                          color: isDark ? AppColors.darkSubtext : AppColors.lightSubtext),
                      const SizedBox(width: 4),
                      Text('${article.readingTime} min read',
                          style: TextStyle(
                              fontSize: 11,
                              color: isDark ? AppColors.darkSubtext : AppColors.lightSubtext)),

                      const Spacer(),

                      // Like
                      _IconAction(
                        icon: _liked
                            ? Icons.favorite_rounded
                            : Icons.favorite_border_rounded,
                        color: _liked ? Colors.red : null,
                        onTap: () async {
                          await HapticFeedback.lightImpact();
                          setState(() => _liked = !_liked);
                          _heartCtrl.forward(from: 0);
                        },
                      ).animate(controller: _heartCtrl)
                          .scale(begin: const Offset(1,1),
                                 end:   const Offset(1.4,1.4),
                                 duration: 150.ms)
                          .then()
                          .scale(begin: const Offset(1.4,1.4),
                                 end:   const Offset(1,1),
                                 duration: 150.ms),

                      const SizedBox(width: 4),

                      // Bookmark
                      _IconAction(
                        icon: _bookmarked
                            ? Icons.bookmark_rounded
                            : Icons.bookmark_border_rounded,
                        color: _bookmarked ? accent : null,
                        onTap: () async {
                          await HapticFeedback.lightImpact();
                          setState(() => _bookmarked = !_bookmarked);
                        },
                      ),

                      const SizedBox(width: 4),

                      // Share
                      _IconAction(
                        icon: Icons.share_rounded,
                        onTap: () => _share(article),
                      ),

                      const SizedBox(width: 4),

                      // Listen (audio)
                      _IconAction(
                        icon: Icons.headphones_rounded,
                        color: accent,
                        onTap: () {},
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  String _timeAgo(String iso) {
    try {
      return timeago.format(DateTime.parse(iso), locale: 'en_short');
    } catch (_) {
      return '';
    }
  }

  void _share(ArticleModel a) {
    // Share via share_plus — implement in service
  }
}

// ── Sub-widgets ────────────────────────────────────────────────────────────────
class _CategoryBadge extends StatelessWidget {
  final String category;
  final Color accent;
  const _CategoryBadge({required this.category, required this.accent});

  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
    decoration: BoxDecoration(
      color: accent.withOpacity(0.85),
      borderRadius: BorderRadius.circular(20),
    ),
    child: Text(category.toUpperCase(),
        style: const TextStyle(color: Colors.white, fontSize: 10,
            fontWeight: FontWeight.w700, letterSpacing: 0.5)),
  );
}

class _BreakingBadge extends StatelessWidget {
  @override
  Widget build(BuildContext context) => Container(
    padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
    decoration: BoxDecoration(
      color: Colors.red.shade600,
      borderRadius: BorderRadius.circular(20),
    ),
    child: Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(width: 6, height: 6,
            decoration: const BoxDecoration(color: Colors.white, shape: BoxShape.circle))
            .animate(onPlay: (c) => c.repeat(reverse: true))
            .fade(begin: 1, end: 0.3, duration: 600.ms),
        const SizedBox(width: 5),
        const Text('BREAKING', style: TextStyle(color: Colors.white,
            fontSize: 10, fontWeight: FontWeight.w800, letterSpacing: 0.5)),
      ],
    ),
  );
}

class _ScopeBadge extends StatelessWidget {
  final String scope; // international / national / state
  const _ScopeBadge({required this.scope});

  @override
  Widget build(BuildContext context) {
    if (scope.isEmpty) return const SizedBox.shrink();
    final (icon, label) = switch (scope) {
      'national'      => ('🇮🇳', 'India'),
      'state'         => ('📍',   'State'),
      _               => ('🌍',   ''),
    };
    if (label.isEmpty) return const SizedBox.shrink();
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 3),
      decoration: BoxDecoration(
        color: Colors.black.withOpacity(0.5),
        borderRadius: BorderRadius.circular(10),
      ),
      child: Text('$icon $label',
          style: const TextStyle(fontSize: 10, color: Colors.white)),
    );
  }
}

class _IconAction extends StatelessWidget {
  final IconData icon;
  final VoidCallback? onTap;
  final Color? color;
  const _IconAction({required this.icon, this.onTap, this.color});

  @override
  Widget build(BuildContext context) => GestureDetector(
    onTap: onTap,
    child: Padding(
      padding: const EdgeInsets.all(4),
      child: Icon(icon, size: 20,
          color: color ?? AppColors.darkSubtext),
    ),
  );
}
