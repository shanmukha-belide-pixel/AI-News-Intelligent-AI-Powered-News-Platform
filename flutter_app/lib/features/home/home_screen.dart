// =============================================================================
// home_screen.dart  —  Premium Dashboard (Sports tab + Weather widget)
// =============================================================================
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_animate/flutter_animate.dart';
import 'package:go_router/go_router.dart';
import 'package:intl/intl.dart';
import '../../core/theme/app_theme.dart';
import '../news/providers/news_provider.dart';
import '../weather/providers/weather_provider.dart';
import '../sports/providers/sports_provider.dart';
import 'widgets/news_card.dart';
import 'widgets/breaking_banner.dart';
import 'widgets/weather_widget.dart';
import 'widgets/sports_ticker.dart';
import 'widgets/category_chips.dart';
import 'widgets/trending_topics.dart';
import 'widgets/scope_tabs.dart';

class HomeScreen extends ConsumerStatefulWidget {
  const HomeScreen({super.key});

  @override
  ConsumerState<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends ConsumerState<HomeScreen>
    with TickerProviderStateMixin {
  late TabController _scopeTab;      // International | National | State | Sports
  late ScrollController _scroll;
  String _selectedState    = '';
  String _selectedCategory = '';
  bool _showWeather        = true;

  static const _tabs = ['🌍 World', '🇮🇳 India', '📍 State', '⚽ Sports'];

  @override
  void initState() {
    super.initState();
    _scopeTab = TabController(length: 4, vsync: this);
    _scroll   = ScrollController();
    // Trigger initial loads
    Future.microtask(() {
      ref.read(internationalNewsProvider.notifier).fetch();
      ref.read(nationalNewsProvider.notifier).fetch();
      ref.read(sportsNewsProvider.notifier).fetch('');
      ref.read(weatherProvider.notifier).fetchCurrentLocation();
      ref.read(breakingNewsProvider.notifier).fetch();
      ref.read(trendingProvider.notifier).fetch();
    });
  }

  @override
  void dispose() {
    _scopeTab.dispose();
    _scroll.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final theme    = Theme.of(context);
    final isDark   = theme.brightness == Brightness.dark;
    final now      = DateTime.now();
    final greeting = _greeting(now.hour);

    return Scaffold(
      backgroundColor: isDark ? AppColors.darkBg : AppColors.lightBg,
      body: NestedScrollView(
        controller: _scroll,
        headerSliverBuilder: (ctx, inner) => [
          // ── App bar ───────────────────────────────────────────────────────
          SliverAppBar(
            expandedHeight: 0,
            floating: true,
            snap: true,
            pinned: false,
            backgroundColor: isDark ? AppColors.darkBg : AppColors.lightBg,
            title: Row(
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(greeting,
                        style: theme.textTheme.labelMedium?.copyWith(
                            color: AppColors.accent)),
                    Text(DateFormat('EEEE, MMM d').format(now),
                        style: theme.textTheme.titleLarge),
                  ],
                ),
                const Spacer(),
                IconButton(
                  icon: const Icon(Icons.search_rounded),
                  onPressed: () => context.push('/search'),
                ),
                IconButton(
                  icon: const Icon(Icons.notifications_outlined),
                  onPressed: () => context.push('/notifications'),
                ),
                GestureDetector(
                  onTap: () => context.push('/profile'),
                  child: const CircleAvatar(
                    radius: 18,
                    backgroundColor: AppColors.primary,
                    child: Icon(Icons.person, size: 18, color: Colors.white),
                  ),
                ),
              ],
            ),
            actions: const [],
          ),

          // ── Scope tabs: World / India / State / Sports ───────────────────
          SliverPersistentHeader(
            pinned: true,
            delegate: _ScopeTabDelegate(
              TabBar(
                controller: _scopeTab,
                isScrollable: true,
                tabAlignment: TabAlignment.start,
                indicator: BoxDecoration(
                  borderRadius: BorderRadius.circular(20),
                  color: AppColors.primary.withOpacity(0.15),
                ),
                labelColor: AppColors.primary,
                unselectedLabelColor:
                    isDark ? AppColors.darkSubtext : AppColors.lightSubtext,
                labelStyle: const TextStyle(fontWeight: FontWeight.w700),
                tabs: _tabs.map((t) => Tab(text: t)).toList(),
              ),
              isDark ? AppColors.darkBg : AppColors.lightBg,
            ),
          ),
        ],

        body: TabBarView(
          controller: _scopeTab,
          children: [
            _InternationalTab(selectedCategory: _selectedCategory,
                onCategoryChange: (c) => setState(()=> _selectedCategory = c)),
            _NationalTab(selectedCategory: _selectedCategory,
                onCategoryChange: (c) => setState(()=> _selectedCategory = c)),
            _StateTab(selectedState: _selectedState,
                onStateChange: (s) => setState(()=> _selectedState = s)),
            _SportsTab(),
          ],
        ),
      ),
    );
  }

  String _greeting(int hour) {
    if (hour < 12) return '☀️ Good Morning';
    if (hour < 17) return '🌤️ Good Afternoon';
    if (hour < 20) return '🌆 Good Evening';
    return '🌙 Good Night';
  }
}

// ── International Tab ─────────────────────────────────────────────────────────
class _InternationalTab extends ConsumerWidget {
  final String selectedCategory;
  final ValueChanged<String> onCategoryChange;
  const _InternationalTab({required this.selectedCategory, required this.onCategoryChange});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final breaking = ref.watch(breakingNewsProvider);
    final news     = ref.watch(internationalNewsProvider);
    final weather  = ref.watch(weatherProvider);
    final trending = ref.watch(trendingProvider);

    return RefreshIndicator(
      color: AppColors.primary,
      onRefresh: () async {
        ref.read(internationalNewsProvider.notifier).fetch(category: selectedCategory);
        ref.read(breakingNewsProvider.notifier).fetch();
      },
      child: CustomScrollView(
        slivers: [
          // Weather widget
          SliverToBoxAdapter(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
              child: WeatherWidget(state: weather),
            ).animate().fadeIn(duration: 400.ms).slideY(begin: -0.2),
          ),

          // Breaking news banner
          if (breaking.hasValue && breaking.value!.isNotEmpty)
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
                child: BreakingBanner(articles: breaking.value!),
              ).animate().fadeIn(duration: 300.ms, delay: 100.ms),
            ),

          // Category filter chips
          SliverToBoxAdapter(
            child: CategoryChips(
              selected: selectedCategory,
              onSelected: onCategoryChange,
            ).animate().fadeIn(delay: 150.ms),
          ),

          // Trending topics
          SliverToBoxAdapter(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(16, 12, 16, 0),
              child: TrendingTopics(trending: trending),
            ).animate().fadeIn(delay: 200.ms),
          ),

          const SliverToBoxAdapter(
            child: Padding(
              padding: EdgeInsets.fromLTRB(16, 20, 16, 8),
              child: Text('Top Headlines',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700)),
            ),
          ),

          // News list
          news.when(
            loading: () => SliverList(
              delegate: SliverChildBuilderDelegate(
                (_, i) => const _SkeletonCard(), childCount: 5),
            ),
            error: (e, _) => SliverToBoxAdapter(
              child: _ErrorState(onRetry: () =>
                  ref.read(internationalNewsProvider.notifier).fetch()),
            ),
            data: (articles) => SliverList(
              delegate: SliverChildBuilderDelegate(
                (ctx, i) => Padding(
                  padding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
                  child: NewsCard(
                    article: articles[i],
                    onTap: () => context.push('/article/${articles[i].id}'),
                  ).animate().fadeIn(delay: (50 * i).ms).slideY(begin: 0.1),
                ),
                childCount: articles.length,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

// ── National Tab (India) ──────────────────────────────────────────────────────
class _NationalTab extends ConsumerWidget {
  final String selectedCategory;
  final ValueChanged<String> onCategoryChange;
  const _NationalTab({required this.selectedCategory, required this.onCategoryChange});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final news = ref.watch(nationalNewsProvider);
    return RefreshIndicator(
      color: AppColors.primary,
      onRefresh: () async =>
          ref.read(nationalNewsProvider.notifier).fetch(category: selectedCategory),
      child: CustomScrollView(
        slivers: [
          SliverToBoxAdapter(
            child: CategoryChips(
              selected: selectedCategory,
              onSelected: onCategoryChange,
              includeLanguageFilter: true,
            ),
          ),
          const SliverToBoxAdapter(
            child: Padding(
              padding: EdgeInsets.fromLTRB(16, 16, 16, 8),
              child: Text('India News',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700)),
            ),
          ),
          news.when(
            loading: () => SliverList(
              delegate: SliverChildBuilderDelegate(
                (_, i) => const _SkeletonCard(), childCount: 5),
            ),
            error: (e, _) => SliverToBoxAdapter(
              child: _ErrorState(onRetry: () =>
                  ref.read(nationalNewsProvider.notifier).fetch()),
            ),
            data: (articles) => SliverList(
              delegate: SliverChildBuilderDelegate(
                (ctx, i) => Padding(
                  padding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
                  child: NewsCard(article: articles[i],
                      onTap: () => context.push('/article/${articles[i].id}')),
                ),
                childCount: articles.length,
              ),
            ),
          ),
        ],
      ),
    );
  }
}

// ── State Tab ─────────────────────────────────────────────────────────────────
class _StateTab extends ConsumerStatefulWidget {
  final String selectedState;
  final ValueChanged<String> onStateChange;
  const _StateTab({required this.selectedState, required this.onStateChange});

  @override
  ConsumerState<_StateTab> createState() => _StateTabState();
}

class _StateTabState extends ConsumerState<_StateTab> {
  static const _states = [
    'Andhra Pradesh','Assam','Bihar','Delhi','Gujarat','Haryana',
    'Jammu & Kashmir','Jharkhand','Karnataka','Kerala','Madhya Pradesh',
    'Maharashtra','Odisha','Punjab','Rajasthan','Tamil Nadu','Telangana',
    'Uttar Pradesh','West Bengal',
  ];

  String _state = 'Maharashtra';

  @override
  void initState() {
    super.initState();
    _state = widget.selectedState.isEmpty ? 'Maharashtra' : widget.selectedState;
    Future.microtask(() =>
        ref.read(stateNewsProvider.notifier).fetch(state: _state));
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final news  = ref.watch(stateNewsProvider);
    final theme = Theme.of(context);

    return Column(
      children: [
        // State picker
        Container(
          height: 44,
          margin: const EdgeInsets.fromLTRB(16, 12, 16, 0),
          decoration: BoxDecoration(
            color: AppColors.darkCard,
            borderRadius: BorderRadius.circular(12),
          ),
          child: DropdownButtonHideUnderline(
            child: DropdownButton<String>(
              value: _state,
              isExpanded: true,
              padding: const EdgeInsets.symmetric(horizontal: 16),
              dropdownColor: AppColors.darkCard,
              items: _states.map((s) => DropdownMenuItem(
                value: s,
                child: Text(s, style: theme.textTheme.bodyMedium),
              )).toList(),
              onChanged: (s) {
                if (s == null) return;
                setState(() => _state = s);
                widget.onStateChange(s);
                ref.read(stateNewsProvider.notifier).fetch(state: s);
              },
            ),
          ),
        ),

        Expanded(
          child: RefreshIndicator(
            color: AppColors.primary,
            onRefresh: () async =>
                ref.read(stateNewsProvider.notifier).fetch(state: _state),
            child: news.when(
              loading: () => ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: 5,
                itemBuilder: (_, __) => const _SkeletonCard(),
              ),
              error: (e, _) => Center(
                child: _ErrorState(onRetry: () =>
                    ref.read(stateNewsProvider.notifier).fetch(state: _state)),
              ),
              data: (articles) => articles.isEmpty
                  ? const Center(child: Text('No news for this state'))
                  : ListView.builder(
                      padding: const EdgeInsets.all(16),
                      itemCount: articles.length,
                      itemBuilder: (ctx, i) => Padding(
                        padding: const EdgeInsets.only(bottom: 12),
                        child: NewsCard(
                          article: articles[i],
                          onTap: () => context.push('/article/${articles[i].id}'),
                        ),
                      ),
                    ),
            ),
          ),
        ),
      ],
    );
  }
}

// ── Sports Tab ────────────────────────────────────────────────────────────────
class _SportsTab extends ConsumerStatefulWidget {
  @override
  ConsumerState<_SportsTab> createState() => _SportsTabState();
}

class _SportsTabState extends ConsumerState<_SportsTab> {
  String _sport = '';

  static const _sports = [
    {'label': 'All',       'value': ''},
    {'label': '🏏 Cricket','value': 'cricket'},
    {'label': '⚽ Football','value': 'football'},
    {'label': '🎾 Tennis', 'value': 'tennis'},
    {'label': '🏀 NBA',    'value': 'nba'},
    {'label': '🏎️ F1',     'value': 'formula'},
    {'label': '🏸 Badminton','value':'badminton'},
    {'label': '🏅 Olympics','value':'olympics'},
  ];

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final news  = ref.watch(sportsNewsProvider);
    final theme = Theme.of(context);

    return Column(
      children: [
        // Sport filter horizontal scroll
        SizedBox(
          height: 44,
          child: ListView.separated(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
            itemCount: _sports.length,
            separatorBuilder: (_, __) => const SizedBox(width: 8),
            itemBuilder: (_, i) {
              final s = _sports[i];
              final isSelected = _sport == s['value'];
              return FilterChip(
                label: Text(s['label']!),
                selected: isSelected,
                selectedColor: AppColors.sports.withOpacity(0.25),
                checkmarkColor: AppColors.sports,
                onSelected: (_) {
                  setState(() => _sport = s['value']!);
                  ref.read(sportsNewsProvider.notifier).fetch(_sport);
                },
              );
            },
          ),
        ),

        // Sports score ticker (placeholder — integrates live scores API)
        const SportsTicker(),

        Expanded(
          child: RefreshIndicator(
            color: AppColors.sports,
            onRefresh: () async =>
                ref.read(sportsNewsProvider.notifier).fetch(_sport),
            child: news.when(
              loading: () => ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: 5,
                itemBuilder: (_, __) => const _SkeletonCard(),
              ),
              error: (e, _) => Center(
                child: _ErrorState(onRetry: () =>
                    ref.read(sportsNewsProvider.notifier).fetch(_sport)),
              ),
              data: (articles) => ListView.builder(
                padding: const EdgeInsets.all(16),
                itemCount: articles.length,
                itemBuilder: (ctx, i) => Padding(
                  padding: const EdgeInsets.only(bottom: 12),
                  child: NewsCard(
                    article: articles[i],
                    accentColor: AppColors.sports,
                    onTap: () => context.push('/article/${articles[i].id}'),
                  ).animate().fadeIn(delay: (40 * i).ms),
                ),
              ),
            ),
          ),
        ),
      ],
    );
  }
}

// ── Skeleton loading card ─────────────────────────────────────────────────────
class _SkeletonCard extends StatelessWidget {
  const _SkeletonCard();

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 0, 16, 12),
      child: Container(
        height: 180,
        decoration: BoxDecoration(
          color: AppColors.darkCard,
          borderRadius: BorderRadius.circular(16),
        ),
      ).animate(onPlay: (c) => c.repeat())
       .shimmer(duration: 1200.ms, color: Colors.white.withOpacity(0.05)),
    );
  }
}

// ── Error state ───────────────────────────────────────────────────────────────
class _ErrorState extends StatelessWidget {
  final VoidCallback onRetry;
  const _ErrorState({required this.onRetry});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(32),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          const Icon(Icons.wifi_off_rounded, size: 64, color: AppColors.darkSubtext),
          const SizedBox(height: 16),
          const Text('Could not load news', style: TextStyle(fontSize: 16)),
          const SizedBox(height: 8),
          const Text('Check your internet connection',
              style: TextStyle(color: AppColors.darkSubtext)),
          const SizedBox(height: 24),
          ElevatedButton.icon(
            onPressed: onRetry,
            icon: const Icon(Icons.refresh),
            label: const Text('Try Again'),
          ),
        ],
      ),
    );
  }
}

// ── Sliver delegate for sticky tab bar ────────────────────────────────────────
class _ScopeTabDelegate extends SliverPersistentHeaderDelegate {
  final TabBar tabBar;
  final Color bg;
  _ScopeTabDelegate(this.tabBar, this.bg);

  @override
  double get minExtent => tabBar.preferredSize.height + 8;
  @override
  double get maxExtent => tabBar.preferredSize.height + 8;

  @override
  Widget build(BuildContext context, double shrinkOffset, bool overlapsContent) {
    return Container(
      color: bg,
      padding: const EdgeInsets.only(top: 4),
      child: tabBar,
    );
  }

  @override
  bool shouldRebuild(_ScopeTabDelegate old) => false;
}
