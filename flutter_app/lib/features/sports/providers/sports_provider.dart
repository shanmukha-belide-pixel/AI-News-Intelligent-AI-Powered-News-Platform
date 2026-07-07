// =============================================================================
// sports_provider.dart  —  Riverpod state management for sports news
// =============================================================================
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../core/network/api_client.dart';
import '../../news/models/article_model.dart';

final sportsNewsProvider =
    StateNotifierProvider<SportsNotifier, AsyncValue<List<ArticleModel>>>((ref) {
  return SportsNotifier();
});

class SportsNotifier extends StateNotifier<AsyncValue<List<ArticleModel>>> {
  SportsNotifier() : super(const AsyncValue.loading());

  Future<void> fetch(String sport) async {
    try {
      state = const AsyncValue.loading();
      final response = await ApiClient.dio.get('/api/v1/news/sports',
          queryParameters: {'sport': sport});
      
      if (response.statusCode == 200 && response.data['success'] == true) {
        final list = (response.data['data'] as List)
            .map((item) => ArticleModel.fromJson(item))
            .toList();
        state = AsyncValue.data(list);
      } else {
        state = AsyncValue.data(_mockSports(sport));
      }
    } catch (_) {
      state = AsyncValue.data(_mockSports(sport));
    }
  }

  List<ArticleModel> _mockSports(String sport) {
    final list = <ArticleModel>[];
    String titlePrefix = sport.isNotEmpty ? '[${sport.toUpperCase()}] ' : '';

    list.add(ArticleModel(
      id: 'sports_1',
      title: '${titlePrefix}India Clinch Historic Victory in Test Match Series Final',
      excerpt: 'A masterclass batting performance on day 5 seals the trophy for the visitors, chasing down a record 324-run target.',
      url: 'https://example.com/sports/cricket',
      imageUrl: 'https://picsum.photos/id/10/600/400',
      publisher: 'Cricbuzz',
      publisherIcon: '',
      author: 'R. Sharma',
      category: 'sports',
      language: 'en',
      country: 'IN',
      state: '',
      scope: 'international',
      publishedAt: DateTime.now().toIso8601String(),
      isBreaking: true,
      readingTime: 4,
      aiSummary: 'India successfully chases down 324 on Day 5 to secure Test Series win.',
    ));

    list.add(ArticleModel(
      id: 'sports_2',
      title: '${titlePrefix}Real Madrid Secure 16th Champions League Crown',
      excerpt: 'A late brace in the second half of extra time breaks the deadlock, cementing their legacy as the kings of Europe.',
      url: 'https://example.com/sports/football',
      imageUrl: 'https://picsum.photos/id/11/600/400',
      publisher: 'Goal.com',
      publisherIcon: '',
      author: 'L. Modric',
      category: 'sports',
      language: 'en',
      country: 'ES',
      state: '',
      scope: 'international',
      publishedAt: DateTime.now().subtract(const Duration(hours: 3)).toIso8601String(),
      isBreaking: false,
      readingTime: 5,
      aiSummary: 'Real Madrid defeats opponents in extra time to win 16th UCL trophy.',
    ));

    list.add(ArticleModel(
      id: 'sports_3',
      title: '${titlePrefix}Hamilton Claims Stunning Home Win at Silverstone Grand Prix',
      excerpt: 'A brilliant wet-weather tire strategy call mid-race allows him to overtake in the final laps, bringing the home crowd to their feet.',
      url: 'https://example.com/sports/f1',
      imageUrl: 'https://picsum.photos/id/12/600/400',
      publisher: 'Formula 1',
      publisherIcon: '',
      author: 'M. Schumacher',
      category: 'sports',
      language: 'en',
      country: 'GB',
      state: '',
      scope: 'international',
      publishedAt: DateTime.now().subtract(const Duration(hours: 6)).toIso8601String(),
      isBreaking: false,
      readingTime: 3,
      aiSummary: 'Hamilton secures British Grand Prix win via superior rain tire strategy.',
    ));

    return list;
  }
}
