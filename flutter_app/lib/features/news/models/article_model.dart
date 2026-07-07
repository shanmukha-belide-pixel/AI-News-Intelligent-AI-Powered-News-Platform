// =============================================================================
// article_model.dart  —  Data model for article metadata
// =============================================================================
import 'package:equatable/equatable.dart';

class ArticleModel extends Equatable {
  final String id;
  final String title;
  final String excerpt;
  final String url;
  final String imageUrl;
  final String publisher;
  final String publisherIcon;
  final String author;
  final String category;
  final String language;
  final String country;
  final String state;
  final String scope;
  final String publishedAt;
  final bool isBreaking;
  final int readingTime;
  final String aiSummary;

  const ArticleModel({
    required this.id,
    required this.title,
    required this.excerpt,
    required this.url,
    required this.imageUrl,
    required this.publisher,
    required this.publisherIcon,
    required this.author,
    required this.category,
    required this.language,
    required this.country,
    required this.state,
    required this.scope,
    required this.publishedAt,
    required this.isBreaking,
    required this.readingTime,
    required this.aiSummary,
  });

  factory ArticleModel.fromJson(Map<String, dynamic> json) {
    return ArticleModel(
      id:            json['id'] ?? '',
      title:         json['title'] ?? '',
      excerpt:       json['excerpt'] ?? '',
      url:           json['url'] ?? '',
      imageUrl:      json['image_url'] ?? '',
      publisher:     json['publisher'] ?? '',
      publisherIcon: json['publisher_icon'] ?? '',
      author:        json['author'] ?? '',
      category:      json['category'] ?? 'general',
      language:      json['language'] ?? 'en',
      country:       json['country'] ?? 'IN',
      state:         json['state'] ?? '',
      scope:         json['scope'] ?? 'international',
      publishedAt:  json['published_at'] ?? '',
      isBreaking:    json['is_breaking'] ?? false,
      readingTime:   json['reading_time'] ?? 2,
      aiSummary:     json['ai_summary'] ?? '',
    );
  }

  Map<String, dynamic> toJson() => {
    'id':             id,
    'title':          title,
    'excerpt':        excerpt,
    'url':            url,
    'image_url':      imageUrl,
    'publisher':      publisher,
    'publisher_icon': publisherIcon,
    'author':         author,
    'category':       category,
    'language':       language,
    'country':        country,
    'state':          state,
    'scope':          scope,
    'published_at':   publishedAt,
    'is_breaking':    isBreaking,
    'reading_time':   readingTime,
    'ai_summary':     aiSummary,
  };

  @override
  List<Object?> get props => [
    id, title, excerpt, url, imageUrl, publisher, publisherIcon,
    author, category, language, country, state, scope, publishedAt,
    isBreaking, readingTime, aiSummary,
  ];
}
