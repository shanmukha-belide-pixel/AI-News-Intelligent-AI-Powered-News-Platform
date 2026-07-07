// =============================================================================
// sports_ticker.dart  —  Horizontal scrolling live scores ticker widget
// =============================================================================
import 'package:flutter/material.dart';
import '../../../../core/theme/app_theme.dart';

class SportsTicker extends StatelessWidget {
  const SportsTicker({super.key});

  @override
  Widget build(BuildContext context) {
    final theme  = Theme.of(context);
    final isDark = theme.brightness == Brightness.dark;

    // Static placeholder scores (can connect to real API later)
    final scores = [
      {'match': 'IPL 2026', 'teams': 'MI vs CSK', 'status': 'Live', 'score': 'CSK 172/4 (18.2)'},
      {'match': 'Premier League', 'teams': 'ARS vs MCI', 'status': 'Full Time', 'score': '2 - 2'},
      {'match': 'Wimbledon', 'teams': 'Sinner vs Alcaraz', 'status': 'Set 3', 'score': '6-4, 3-6, 4-2'},
      {'match': 'NBA Finals', 'teams': 'BOS vs DAL', 'status': 'Q3', 'score': '88 - 84'},
      {'match': 'Formula 1', 'teams': 'Italian GP', 'status': 'Completed', 'score': 'LEC (P1), PIA (P2)'},
    ];

    return Container(
      height: 72,
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: ListView.separated(
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.symmetric(horizontal: 16),
        itemCount: scores.length,
        separatorBuilder: (_, __) => const SizedBox(width: 10),
        itemBuilder: (context, i) {
          final s = scores[i];
          final isLive = s['status'] == 'Live' || s['status']!.startsWith('Set') || s['status']!.startsWith('Q');
          
          return Container(
            padding: const EdgeInsets.all(10),
            decoration: BoxDecoration(
              color: isDark ? AppColors.darkCard : AppColors.lightCard,
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: isDark ? AppColors.darkBorder : AppColors.lightBorder,
                width: 0.5,
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Row(
                  children: [
                    Text(s['match']!,
                        style: theme.textTheme.labelSmall?.copyWith(
                            color: AppColors.sports, fontWeight: FontWeight.w600)),
                    const SizedBox(width: 8),
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 1),
                      decoration: BoxDecoration(
                        color: isLive ? Colors.red.withOpacity(0.15) : Colors.grey.withOpacity(0.15),
                        borderRadius: BorderRadius.circular(4),
                      ),
                      child: Text(
                        s['status']!.toUpperCase(),
                        style: TextStyle(
                          color: isLive ? Colors.red : Colors.grey,
                          fontSize: 8,
                          fontWeight: FontWeight.w700,
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 4),
                Row(
                  children: [
                    Text(s['teams']!,
                        style: theme.textTheme.titleSmall?.copyWith(
                            fontWeight: FontWeight.w700)),
                    const SizedBox(width: 8),
                    Text(s['score']!,
                        style: theme.textTheme.bodySmall?.copyWith(
                            color: isDark ? Colors.white70 : Colors.black87,
                            fontWeight: FontWeight.w500)),
                  ],
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
